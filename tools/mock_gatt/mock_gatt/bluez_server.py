from __future__ import annotations

import argparse
import json
import sys

from .core import CHARACTERISTIC_UUIDS, SERVICE_UUIDS, FixtureRig, SimulatedDisconnect, _stable_json_bytes


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hornet Snapper R3 mock GATT server shell")
    parser.add_argument("--adapter", default="hci0", help="BlueZ adapter name for manual Pi5 runs")
    parser.add_argument("--dry-run", action="store_true", help="print the v0 GATT contract and exit")
    parser.add_argument("--manifest-encoding", choices=("cbor", "json"), default="cbor")
    args = parser.parse_args(argv)

    rig = FixtureRig()
    if args.dry_run:
        _print_contract(rig, args.manifest_encoding)
        return 0

    return _serve_bluez(args.adapter, rig, args.manifest_encoding)


def _print_contract(rig: FixtureRig, manifest_encoding: str) -> None:
    print("Hornet Snapper R3 v0 mock GATT")
    for name, uuid in SERVICE_UUIDS.items():
        print(f"{name} {uuid}")
    for name, uuid in CHARACTERISTIC_UUIDS.items():
        print(f"  {name} {uuid}")
    manifest = json.loads(rig.manifest_bytes("json").decode("utf-8"))
    print(f"manifest encoding: {manifest_encoding}")
    print("FILE_REQ wire: JSON {'file': str, 'offset': int}")
    print("FILE_DATA wire: raw bytes, <=244 bytes; chunk CRC comes from manifest.chunk_crc16")
    print("ACK_DELETE wire: JSON {'file': str}")
    print("FEEDBACK wire: JSON {'file': str, 'verdict': 'good-fire'|'false-positive', 'true_class'?: str}")
    print(f"fixture rig: {manifest['rig_serial']} {manifest['fw']}")
    print(f"fixture files: {len(manifest['files'])}")


def _serve_bluez(adapter: str, rig: FixtureRig, manifest_encoding: str) -> int:
    try:
        import dbus
        import dbus.exceptions
        import dbus.mainloop.glib
        import dbus.service
        from gi.repository import GLib
    except Exception as exc:
        print(
            "BlueZ D-Bus dependencies are missing. Install python3-dbus and "
            f"python3-gi on the Pi5, then retry. ({exc})"
        )
        return 2

    bluez = "org.bluez"
    object_manager_iface = "org.freedesktop.DBus.ObjectManager"
    properties_iface = "org.freedesktop.DBus.Properties"
    gatt_manager_iface = "org.bluez.GattManager1"
    gatt_service_iface = "org.bluez.GattService1"
    gatt_char_iface = "org.bluez.GattCharacteristic1"
    le_ad_manager_iface = "org.bluez.LEAdvertisingManager1"
    le_advertisement_iface = "org.bluez.LEAdvertisement1"
    adapter_path = f"/org/bluez/{adapter}"

    def dbus_bytes(payload: bytes):
        return dbus.Array([dbus.Byte(byte) for byte in payload], signature="y")

    class TransferState:
        def __init__(self):
            self.iterator = None
            self.error = None

        def set_request(self, payload: bytes) -> None:
            req = json.loads(payload.decode("utf-8"))
            filename = req.get("file") or req.get("filename")
            offset = int(req.get("offset", 0))
            self.iterator = iter(rig.request_file(filename, offset))
            self.error = None

        def next_payload(self) -> bytes:
            if self.iterator is None:
                return b""
            try:
                return _file_data_payload(next(self.iterator))
            except StopIteration:
                self.iterator = None
                return b""
            except SimulatedDisconnect as exc:
                self.iterator = None
                self.error = exc
                return b""

    transfer = TransferState()

    class Application(dbus.service.Object):
        path = "/com/hornetsnapper/mock_gatt"

        def __init__(self, bus):
            self.services = []
            super().__init__(bus, self.path)
            self.services.append(Service(bus, 0, SERVICE_UUIDS["HS-INFO"]))
            self.services[-1].add_characteristic(
                Characteristic(bus, 0, CHARACTERISTIC_UUIDS["DEVICE_INFO"], ["read"], self.services[-1], read_fn=rig.device_info_bytes)
            )
            self.services[-1].add_characteristic(
                Characteristic(
                    bus,
                    1,
                    CHARACTERISTIC_UUIDS["BATTERY"],
                    ["read"],
                    self.services[-1],
                    read_fn=lambda: _stable_json_bytes(rig.status.battery.to_dict()),
                )
            )
            self.services[-1].add_characteristic(
                Characteristic(bus, 2, CHARACTERISTIC_UUIDS["STATUS"], ["read"], self.services[-1], read_fn=rig.status_bytes)
            )
            self.services.append(Service(bus, 1, SERVICE_UUIDS["HS-SYNC"]))
            self.services[-1].add_characteristic(
                Characteristic(
                    bus,
                    0,
                    CHARACTERISTIC_UUIDS["LIST_INDEX"],
                    ["read"],
                    self.services[-1],
                    read_fn=lambda: rig.manifest_bytes(manifest_encoding),
                )
            )
            self.services[-1].add_characteristic(
                Characteristic(bus, 1, CHARACTERISTIC_UUIDS["FILE_REQ"], ["write"], self.services[-1], write_fn=transfer.set_request)
            )
            self.services[-1].add_characteristic(
                Characteristic(
                    bus,
                    2,
                    CHARACTERISTIC_UUIDS["FILE_DATA"],
                    ["read", "indicate"],
                    self.services[-1],
                    read_fn=transfer.next_payload,
                )
            )
            self.services[-1].add_characteristic(
                Characteristic(
                    bus,
                    3,
                    CHARACTERISTIC_UUIDS["ACK_DELETE"],
                    ["write"],
                    self.services[-1],
                    write_fn=lambda payload: rig.ack_delete(_decode_file_payload(payload)),
                )
            )
            self.services.append(Service(bus, 2, SERVICE_UUIDS["HS-FB"]))
            self.services[-1].add_characteristic(
                Characteristic(
                    bus,
                    0,
                    CHARACTERISTIC_UUIDS["FEEDBACK"],
                    ["write"],
                    self.services[-1],
                    write_fn=lambda payload: rig.capture_feedback(json.loads(payload.decode("utf-8"))),
                )
            )

        @dbus.service.method(object_manager_iface, out_signature="a{oa{sa{sv}}}")
        def GetManagedObjects(self):
            managed = {}
            for service in self.services:
                managed[service.get_path()] = service.get_properties()
                for char in service.characteristics:
                    managed[char.get_path()] = char.get_properties()
            return managed

    class Advertisement(dbus.service.Object):
        path = "/com/hornetsnapper/mock_gatt/advertisement0"

        def __init__(self, bus):
            super().__init__(bus, self.path)

        def get_path(self):
            return dbus.ObjectPath(self.path)

        def get_properties(self):
            return {
                le_advertisement_iface: {
                    "Type": "peripheral",
                    "ServiceUUIDs": dbus.Array([SERVICE_UUIDS["HS-INFO"]], signature="s"),
                    "LocalName": "HornetSnapper-R3",
                }
            }

        @dbus.service.method(properties_iface, in_signature="s", out_signature="a{sv}")
        def GetAll(self, interface):
            if interface != le_advertisement_iface:
                raise dbus.exceptions.DBusException("org.bluez.Error.InvalidArgs")
            return self.get_properties()[le_advertisement_iface]

        @dbus.service.method(le_advertisement_iface)
        def Release(self):
            print("Hornet Snapper R3 advertisement released")

    class Service(dbus.service.Object):
        def __init__(self, bus, index: int, uuid: str):
            self.path = f"{Application.path}/service{index}"
            self.uuid = uuid
            self.characteristics = []
            super().__init__(bus, self.path)

        def get_path(self):
            return dbus.ObjectPath(self.path)

        def add_characteristic(self, characteristic):
            self.characteristics.append(characteristic)

        def get_properties(self):
            return {
                gatt_service_iface: {
                    "UUID": self.uuid,
                    "Primary": True,
                    "Characteristics": dbus.Array([char.get_path() for char in self.characteristics], signature="o"),
                }
            }

        @dbus.service.method(properties_iface, in_signature="s", out_signature="a{sv}")
        def GetAll(self, interface):
            if interface != gatt_service_iface:
                raise dbus.exceptions.DBusException("org.bluez.Error.InvalidArgs")
            return self.get_properties()[gatt_service_iface]

    class Characteristic(dbus.service.Object):
        def __init__(self, bus, index: int, uuid: str, flags: list[str], service: Service, read_fn=None, write_fn=None):
            self.path = f"{service.path}/char{index}"
            self.uuid = uuid
            self.flags = flags
            self.service = service
            self.read_fn = read_fn
            self.write_fn = write_fn
            self.notifying = False
            super().__init__(bus, self.path)

        def get_path(self):
            return dbus.ObjectPath(self.path)

        def get_properties(self):
            return {
                gatt_char_iface: {
                    "UUID": self.uuid,
                    "Service": self.service.get_path(),
                    "Flags": dbus.Array(self.flags, signature="s"),
                    "Notifying": self.notifying,
                }
            }

        @dbus.service.method(properties_iface, in_signature="s", out_signature="a{sv}")
        def GetAll(self, interface):
            if interface != gatt_char_iface:
                raise dbus.exceptions.DBusException("org.bluez.Error.InvalidArgs")
            return self.get_properties()[gatt_char_iface]

        @dbus.service.method(gatt_char_iface, in_signature="a{sv}", out_signature="ay")
        def ReadValue(self, options):
            if self.read_fn is None:
                raise dbus.exceptions.DBusException("org.bluez.Error.NotPermitted")
            payload = self.read_fn()
            offset = int(options.get("offset", 0)) if options else 0
            return dbus_bytes(payload[offset:offset + 244])

        @dbus.service.method(gatt_char_iface, in_signature="aya{sv}")
        def WriteValue(self, value, options):
            if self.write_fn is None:
                raise dbus.exceptions.DBusException("org.bluez.Error.NotPermitted")
            self.write_fn(bytes(int(byte) for byte in value))

        @dbus.service.method(gatt_char_iface)
        def StartNotify(self):
            self.notifying = True

        @dbus.service.method(gatt_char_iface)
        def StopNotify(self):
            self.notifying = False

    def register_ok():
        print(f"Serving Hornet Snapper R3 mock GATT on BlueZ adapter {adapter}")
        _print_contract(rig, manifest_encoding)

    def advertise_ok():
        print(f"Advertising HornetSnapper-R3 on BlueZ adapter {adapter}")

    def register_error(error):
        print(f"Failed to register mock GATT application on {adapter}: {error}")
        loop.quit()

    def advertise_error(error):
        print(f"Failed to register mock GATT advertisement on {adapter}: {error}")
        loop.quit()

    def _file_data_payload(chunk) -> bytes:
        if len(chunk.data) > 244:
            raise ValueError(f"FILE_DATA payload exceeds 244 bytes: {len(chunk.data)}")
        return chunk.data

    def _decode_file_payload(payload: bytes) -> str:
        req = json.loads(payload.decode("utf-8"))
        filename = req.get("file") or req.get("filename")
        if not isinstance(filename, str):
            raise ValueError("ACK_DELETE requires file or filename")
        return filename

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    loop = GLib.MainLoop()
    try:
        adapter_obj = bus.get_object(bluez, adapter_path)
        manager = dbus.Interface(adapter_obj, gatt_manager_iface)
        advertiser = dbus.Interface(adapter_obj, le_ad_manager_iface)
        app = Application(bus)
        advertisement = Advertisement(bus)
        manager.RegisterApplication(app.path, {}, reply_handler=register_ok, error_handler=register_error)
        advertiser.RegisterAdvertisement(
            advertisement.get_path(),
            {},
            reply_handler=advertise_ok,
            error_handler=advertise_error,
        )
    except Exception as exc:
        print(f"Unable to start BlueZ mock GATT on {adapter_path}: {exc}")
        return 2

    try:
        loop.run()
    except KeyboardInterrupt:
        print("Stopping Hornet Snapper R3 mock GATT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
