import io
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from mock_gatt import bluez_server


class BlueZShellTests(unittest.TestCase):
    def test_dry_run_prints_contract_without_adapter(self):
        out = io.StringIO()

        with redirect_stdout(out):
            rc = bluez_server.main(["--dry-run"])

        self.assertEqual(rc, 0)
        text = out.getvalue()
        self.assertIn("HS-INFO 0000fe40-0000-1000-8000-00805f9b34fb", text)
        self.assertIn("LIST_INDEX", text)
        self.assertIn("FILE_DATA wire: raw bytes", text)
        self.assertIn("fixture files: 2", text)

    def test_run_without_bluez_dependency_returns_clear_error(self):
        out = io.StringIO()

        with redirect_stdout(out):
            rc = bluez_server.main(["--adapter", "hci-does-not-exist"])

        self.assertEqual(rc, 2)
        self.assertRegex(out.getvalue(), "BlueZ D-Bus dependencies are missing|Unable to start BlueZ mock GATT")

    def test_server_source_registers_bluez_gatt_application(self):
        source = Path(bluez_server.__file__).read_text(encoding="utf-8")

        self.assertIn("RegisterApplication", source)
        self.assertIn("RegisterAdvertisement", source)
        self.assertIn("org.bluez.LEAdvertisingManager1", source)
        self.assertIn("org.bluez.LEAdvertisement1", source)
        self.assertIn("org.bluez.GattService1", source)
        self.assertIn("org.bluez.GattCharacteristic1", source)
        self.assertIn('"ServiceUUIDs": dbus.Array([SERVICE_UUIDS["HS-INFO"]]', source)
        self.assertNotIn("tx-power", source)

    def test_file_data_path_returns_raw_payload_not_json_hex(self):
        source = Path(bluez_server.__file__).read_text(encoding="utf-8")

        self.assertIn("return chunk.data", source)
        self.assertIn("FILE_DATA payload exceeds 244 bytes", source)
        self.assertIn('options.get("offset", 0)', source)
        self.assertIn("payload[offset:offset + 244]", source)


if __name__ == "__main__":
    unittest.main()
