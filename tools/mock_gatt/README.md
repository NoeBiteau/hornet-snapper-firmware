# Hornet Snapper R3 Mock GATT

Python tooling for the R3 v0 Android BLE sync mock rig. The package provides:

- deterministic HS-INFO, HS-SYNC, and HS-FB UUID constants in the 0xFE40 range;
- deterministic multi-rig fixtures with one selected rig served at a time;
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
summary plus the v0 wire payload contract. It also prints the selected rig
serial and fleet count. It is safe on developer machines without Bluetooth
hardware.

To list deterministic fixtures:

```bash
python -m mock_gatt.bluez_server --list-rigs
```

To inspect a specific scenario or rig:

```bash
python -m mock_gatt.bluez_server --dry-run --scenario low-battery
python -m mock_gatt.bluez_server --dry-run --rig HS-PI5-MOCK-0003
```

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

The BlueZ shell serves one selected rig on one adapter at a time. Use
`--scenario {default,low-battery,stale-calibration,empty}` or `--rig SERIAL`
to choose which fixture is advertised and exposed through GATT. `--rig` selects
the exact serial when both options are present.

## Fixture Behavior

The core mock exposes:

- `fixture_fleet()`, `find_fixture_rig(serial)`, and `select_fixture_rig(...)`
  for deterministic R3.1 fleet selection;
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

The fleet currently includes:

- `HS-PI5-MOCK-0001` default healthy rig with two fixture files;
- `HS-PI5-MOCK-0002` low-battery rig with the same v0 manifest shape;
- `HS-PI5-MOCK-0003` stale-calibration rig;
- `HS-PI5-MOCK-0004` empty rig with no clips.

The v0 UUIDs and payload schema are unchanged across all fixtures.

The default manifest mode emits deterministic CBOR using the small encoder in
`mock_gatt.core`; JSON is still available for dry-run inspection and tests.
