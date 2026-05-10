# Hornet Snapper R3 Mock GATT

Python tooling for the R3 v0 Android BLE sync mock rig. The package provides:

- deterministic HS-INFO, HS-SYNC, and HS-FB UUID constants in the 0xFE40 range;
- fixture rig info, status, manifest, file chunks, ACK_DELETE, and feedback capture;
- standard-library unit tests that do not require a Bluetooth adapter;
- a BlueZ-facing command shell for Pi5 manual validation planning.

## Run Tests

From the repository root:

```bash
python -m unittest discover firmware/tools/mock_gatt/tests
```

## Inspect The Mock Contract

From this folder:

```bash
python -m mock_gatt.bluez_server --dry-run
```

The dry run prints service UUIDs, characteristic UUIDs, and fixture manifest
summary plus the v0 wire payload contract. It is safe on developer machines
without Bluetooth hardware.

## Manual Pi5 Command

On a Raspberry Pi 5 with BlueZ enabled:

```bash
cd firmware/tools/mock_gatt
python -m mock_gatt.bluez_server --adapter hci0
```

The server uses BlueZ's D-Bus GATT API through the optional `python3-dbus` and
`python3-gi` packages. On Pi OS install them first:

```bash
sudo apt-get install -y python3-dbus python3-gi bluez
```

If those packages or the requested adapter are missing, the command exits with a
clear setup error. Unit tests do not require these packages or Bluetooth
hardware.

The LE advertisement intentionally includes only the primary HS-INFO UUID to
fit legacy advertising payload limits. HS-SYNC and HS-FB are exposed through
normal GATT service discovery after the Android app connects.

## Fixture Behavior

The core mock exposes:

- `DEVICE_INFO`, `BATTERY`, and `STATUS` JSON payloads for HS-INFO;
- a manifest payload for `LIST_INDEX`;
- `FILE_REQ` behavior through `FixtureRig.request_file(filename, offset)`;
- 244-byte maximum chunk payloads with offset, EOF, CRC16 chunk metadata, and
  whole-file CRC32 metadata;
- BlueZ `FILE_DATA` reads return raw chunk bytes within the 244-byte payload
  budget; manifest metadata carries per-chunk CRC16 values for verification;
- `FILE_REQ`, `ACK_DELETE`, and `FEEDBACK` are compact JSON writes. Android
  mirrors these encoders in `:core:ble` so request/delete/feedback vectors stay
  aligned with the mock;
- `disconnect_after_chunks` to simulate mid-transfer disconnect and resume;
- `ACK_DELETE` recording without deleting deterministic fixture bytes;
- HS-FB feedback validation and capture using `good-fire` and
  `false-positive` verdict tags.

The default manifest mode emits deterministic CBOR using the small encoder in
`mock_gatt.core`; JSON is still available for dry-run inspection and tests.
