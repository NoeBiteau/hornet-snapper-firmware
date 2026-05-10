import io
import sys
import unittest
from contextlib import redirect_stderr, redirect_stdout
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
        self.assertIn("selected rig: HS-PI5-MOCK-0001", text)
        self.assertIn("fixture fleet: 4 rigs", text)
        self.assertIn("fixture files: 2", text)

    def test_list_rigs_prints_all_fixture_serials_without_adapter(self):
        out = io.StringIO()

        with redirect_stdout(out):
            rc = bluez_server.main(["--list-rigs"])

        self.assertEqual(rc, 0)
        text = out.getvalue()
        self.assertIn("HS-PI5-MOCK-0001 default", text)
        self.assertIn("HS-PI5-MOCK-0002 low-battery", text)
        self.assertIn("HS-PI5-MOCK-0003 stale-calibration", text)
        self.assertIn("HS-PI5-MOCK-0004 empty", text)

    def test_dry_run_can_select_rig_by_serial(self):
        out = io.StringIO()

        with redirect_stdout(out):
            rc = bluez_server.main(["--dry-run", "--rig", "HS-PI5-MOCK-0003"])

        self.assertEqual(rc, 0)
        self.assertIn("selected rig: HS-PI5-MOCK-0003", out.getvalue())
        self.assertIn("fixture files: 2", out.getvalue())

    def test_dry_run_can_select_scenario(self):
        out = io.StringIO()

        with redirect_stdout(out):
            rc = bluez_server.main(["--dry-run", "--scenario", "empty"])

        self.assertEqual(rc, 0)
        self.assertIn("selected rig: HS-PI5-MOCK-0004", out.getvalue())
        self.assertIn("fixture files: 0", out.getvalue())

    def test_invalid_rig_returns_nonzero_with_clear_message(self):
        err = io.StringIO()

        with redirect_stderr(err):
            rc = bluez_server.main(["--dry-run", "--rig", "HS-PI5-MOCK-9999"])

        self.assertEqual(rc, 2)
        self.assertIn("unknown fixture rig: HS-PI5-MOCK-9999", err.getvalue())

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
