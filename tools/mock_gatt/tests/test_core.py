import json
import sys
import unittest
import binascii
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from mock_gatt import core


class GattConstantsTests(unittest.TestCase):
    def test_services_use_deterministic_fe40_uuid_map(self):
        self.assertEqual(
            core.SERVICE_UUIDS,
            {
                "HS-INFO": "0000fe40-0000-1000-8000-00805f9b34fb",
                "HS-SYNC": "0000fe41-0000-1000-8000-00805f9b34fb",
                "HS-FB": "0000fe42-0000-1000-8000-00805f9b34fb",
            },
        )
        self.assertEqual(core.CHARACTERISTIC_UUIDS["DEVICE_INFO"], "0000fe43-0000-1000-8000-00805f9b34fb")
        self.assertEqual(core.CHARACTERISTIC_UUIDS["ACK_DELETE"], "0000fe49-0000-1000-8000-00805f9b34fb")
        self.assertEqual(core.CHARACTERISTIC_UUIDS["FEEDBACK"], "0000fe4a-0000-1000-8000-00805f9b34fb")


class FixtureRigTests(unittest.TestCase):
    def test_manifest_json_contains_crc_metadata_and_overlay_fields(self):
        rig = core.FixtureRig()

        manifest = json.loads(rig.manifest_bytes("json").decode("utf-8"))

        self.assertEqual(manifest["rig_serial"], "HS-PI5-MOCK-0001")
        self.assertEqual(manifest["fw"], "mock-r3-v0")
        self.assertEqual(manifest["battery"]["v_mv"], 3782)
        self.assertGreaterEqual(len(manifest["files"]), 2)
        first = manifest["files"][0]
        expected_crc = binascii.crc32(rig.file_bytes(first["name"])) & 0xFFFFFFFF
        self.assertEqual(first["crc32"], f"0x{expected_crc:08X}")
        self.assertEqual(first["chunk_size"], core.CHUNK_PAYLOAD_BYTES)
        self.assertEqual(first["chunk_crc16"][0], "0x983A")
        self.assertEqual(first["bbox"], {"x": 0.42, "y": 0.31, "w": 0.18, "h": 0.16})
        self.assertEqual(first["size"], len(rig.file_bytes(first["name"])))

    def test_manifest_cbor_mode_is_binary_cbor_not_json_fallback(self):
        rig = core.FixtureRig()

        payload = rig.manifest_bytes("cbor")

        self.assertFalse(payload.startswith(b"HSJSON0\n"))
        self.assertNotEqual(payload[:1], b"{")
        self.assertIn(b"hs.r3.gatt.v0", payload)

    def test_file_request_slices_244_byte_chunks_from_offset(self):
        rig = core.FixtureRig()
        file_name = rig.manifest.files[0].name

        chunks = list(rig.request_file(file_name, offset=244))

        self.assertEqual(chunks[0].offset, 244)
        self.assertLessEqual(len(chunks[0].data), core.CHUNK_PAYLOAD_BYTES)
        rebuilt = b"".join(chunk.data for chunk in chunks)
        self.assertEqual(rebuilt, rig.file_bytes(file_name)[244:])
        self.assertEqual(chunks[0].chunk_crc16, 0xF387)
        self.assertTrue(chunks[0].file_crc32.startswith("0x"))
        self.assertEqual(chunks[-1].eof, True)

    def test_disconnect_hook_stops_stream_and_resume_continues_at_last_offset(self):
        rig = core.FixtureRig()
        file_name = rig.manifest.files[0].name
        rig.disconnect_after_chunks = 1

        with self.assertRaises(core.SimulatedDisconnect) as ctx:
            list(rig.request_file(file_name, offset=0))

        resume_offset = ctx.exception.resume_offset
        self.assertEqual(resume_offset, core.CHUNK_PAYLOAD_BYTES)
        rig.disconnect_after_chunks = None
        resumed = b"".join(chunk.data for chunk in rig.request_file(file_name, offset=resume_offset))
        self.assertEqual(rig.file_bytes(file_name)[resume_offset:], resumed)

    def test_ack_delete_marks_file_acknowledged_without_removing_fixture_bytes(self):
        rig = core.FixtureRig()
        file_name = rig.manifest.files[0].name

        self.assertTrue(rig.ack_delete(file_name))

        self.assertIn(file_name, rig.acked_deletes)
        self.assertGreater(len(rig.file_bytes(file_name)), 0)

    def test_feedback_capture_validates_and_records_payloads(self):
        rig = core.FixtureRig()
        file_name = rig.manifest.files[0].name

        item = rig.capture_feedback(
            {
                "file": file_name,
                "verdict": "false-positive",
                "true_class": "vespa_crabro",
                "notes": "reviewed on mock",
                "ts_review": 1778256000,
            }
        )

        self.assertEqual(item.verdict, "false-positive")
        self.assertEqual(len(rig.feedback_items), 1)
        rig.capture_feedback({"file": file_name, "verdict": "good-fire"})
        self.assertEqual(len(rig.feedback_items), 2)
        with self.assertRaises(ValueError):
            rig.capture_feedback({"file": file_name, "verdict": "maybe"})


if __name__ == "__main__":
    unittest.main()
