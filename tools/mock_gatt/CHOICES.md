# Choices: R3 Mock GATT

## Purpose

This folder owns the Python-side R3 v0 mock GATT tooling used to validate the
Android BLE sync flow against deterministic rig fixtures.

## Decisions

- UUIDs are deterministic Bluetooth-base UUIDs mapped from the 0xFE40 range:
  HS-INFO at 0xFE40, HS-SYNC at 0xFE41, HS-FB at 0xFE42, and characteristics at
  0xFE43 through 0xFE4A.
- Core behavior is pure Python standard library so CI and developer tests do
  not need a Bluetooth adapter.
- Fixture files are generated in memory with stable CRC32 metadata, bbox fields,
  and Android v0 manifest fields.
- The mock supports offset-based file reads, 244-byte chunks, simulated
  disconnect/resume, ACK_DELETE recording, and feedback capture.
- BlueZ `FILE_DATA` returns raw bytes, not JSON, so it stays inside the 244-byte
  payload budget. Manifest entries carry per-chunk CRC16 metadata.
- Android mirrors this exact v0 payload contract in `:core:ble`
  (`GattPayloadCodec`, `GattManifestCodec`, and `GattRigConnection`) so mock
  request/delete/feedback vectors do not drift from the Pi5 server.
- Manifests default to deterministic CBOR from a tiny local encoder. JSON stays
  available only for dry-run readability.
- The BlueZ entry point uses BlueZ's D-Bus GATT API when `python3-dbus`,
  `python3-gi`, and a local adapter are present. It registers both a GATT app
  and a compact LE advertisement with the primary HS-INFO UUID; HS-SYNC and
  HS-FB are discovered after connection. The fixture core remains
  standard-library and adapter-free for CI.

## Known Issues

- TD-004: this is not ESP32-C3 NimBLE. The mock validates the shared contract
  and sync behavior, but real C3 verification remains R4 work.
- The D-Bus server path is smoke-tested by source inspection and dry-run output
  only in normal CI; exercising live advertisement/connection still requires a
  Pi5 with BlueZ.
- BLE throughput is not measured here. Chunk sizing is represented, but radio
  timing and 2M PHY behavior require hardware validation.

## Interfaces

- Tests: `python -m unittest discover firmware/tools/mock_gatt/tests`
- Dry run: `python -m mock_gatt.bluez_server --dry-run`
- Pi5 shell: `python -m mock_gatt.bluez_server --adapter hci0`
