from __future__ import annotations

import binascii
import json
from dataclasses import dataclass, field
from typing import Iterable


BASE_UUID_SUFFIX = "0000-1000-8000-00805f9b34fb"
CHUNK_PAYLOAD_BYTES = 244

SERVICE_UUIDS = {
    "HS-INFO": f"0000fe40-{BASE_UUID_SUFFIX}",
    "HS-SYNC": f"0000fe41-{BASE_UUID_SUFFIX}",
    "HS-FB": f"0000fe42-{BASE_UUID_SUFFIX}",
}

CHARACTERISTIC_UUIDS = {
    "DEVICE_INFO": f"0000fe43-{BASE_UUID_SUFFIX}",
    "BATTERY": f"0000fe44-{BASE_UUID_SUFFIX}",
    "STATUS": f"0000fe45-{BASE_UUID_SUFFIX}",
    "LIST_INDEX": f"0000fe46-{BASE_UUID_SUFFIX}",
    "FILE_REQ": f"0000fe47-{BASE_UUID_SUFFIX}",
    "FILE_DATA": f"0000fe48-{BASE_UUID_SUFFIX}",
    "ACK_DELETE": f"0000fe49-{BASE_UUID_SUFFIX}",
    "FEEDBACK": f"0000fe4a-{BASE_UUID_SUFFIX}",
}

VALID_VERDICTS = {"good-fire", "false-positive"}
VALID_TRUE_CLASSES = {"vespa_velutina", "vespa_crabro", "apis_mellifera", "other", None}


@dataclass(frozen=True)
class Battery:
    v_mv: int
    soc_pct: int
    charging: bool

    def to_dict(self) -> dict:
        return {"v_mv": self.v_mv, "soc_pct": self.soc_pct, "charging": self.charging}


@dataclass(frozen=True)
class RigInfo:
    rig_serial: str
    fw: str
    model: str
    hw_rev: str

    def to_dict(self) -> dict:
        return {
            "rig_serial": self.rig_serial,
            "fw": self.fw,
            "model": self.model,
            "hw_rev": self.hw_rev,
        }


@dataclass(frozen=True)
class RigStatus:
    uptime_s: int
    sd_free_mb: int
    battery: Battery
    last_detection_ts: int
    events_total: int
    calibration_status: str

    def to_dict(self) -> dict:
        return {
            "uptime_s": self.uptime_s,
            "sd_free_mb": self.sd_free_mb,
            "battery": self.battery.to_dict(),
            "last_detection_ts": self.last_detection_ts,
            "events_total": self.events_total,
            "calibration_status": self.calibration_status,
        }


@dataclass(frozen=True)
class ClipFile:
    name: str
    data: bytes
    ts_unix: int
    primary_class: str
    max_conf: float
    n_detections: int
    thumbnail_idx: int
    bbox: dict[str, float]

    @property
    def size(self) -> int:
        return len(self.data)

    @property
    def crc32_int(self) -> int:
        return binascii.crc32(self.data) & 0xFFFFFFFF

    @property
    def crc32_hex(self) -> str:
        return f"0x{self.crc32_int:08X}"

    def to_manifest_dict(self) -> dict:
        return {
            "name": self.name,
            "size": self.size,
            "crc32": self.crc32_hex,
            "chunk_size": CHUNK_PAYLOAD_BYTES,
            "chunk_crc16": [
                f"0x{_crc16_ccitt(self.data[i:i + CHUNK_PAYLOAD_BYTES]):04X}"
                for i in range(0, self.size, CHUNK_PAYLOAD_BYTES)
            ],
            "ts_unix": self.ts_unix,
            "primary_class": self.primary_class,
            "max_conf": self.max_conf,
            "n_detections": self.n_detections,
            "thumbnail_idx": self.thumbnail_idx,
            "bbox": self.bbox,
        }


@dataclass(frozen=True)
class ClipManifest:
    rig_info: RigInfo
    status: RigStatus
    files: tuple[ClipFile, ...]
    schema: str = "hs.r3.gatt.v0"

    def to_dict(self) -> dict:
        return {
            "schema": self.schema,
            "rig_serial": self.rig_info.rig_serial,
            "fw": self.rig_info.fw,
            "model": self.rig_info.model,
            "sd_free_mb": self.status.sd_free_mb,
            "battery": self.status.battery.to_dict(),
            "events_total": self.status.events_total,
            "calibration_status": self.status.calibration_status,
            "files": [clip.to_manifest_dict() for clip in self.files],
        }


@dataclass(frozen=True)
class Chunk:
    filename: str
    offset: int
    data: bytes
    eof: bool
    file_crc32: str
    chunk_crc16: int

    def to_wire_dict(self) -> dict:
        return {
            "filename": self.filename,
            "offset": self.offset,
            "eof": self.eof,
            "file_crc32": self.file_crc32,
            "chunk_crc16": self.chunk_crc16,
            "data_hex": self.data.hex(),
        }


@dataclass(frozen=True)
class FeedbackItem:
    file: str
    verdict: str
    true_class: str | None = None
    notes: str = ""
    ts_review: int | None = None

    def to_dict(self) -> dict:
        return {
            "file": self.file,
            "verdict": self.verdict,
            "true_class": self.true_class,
            "notes": self.notes,
            "ts_review": self.ts_review,
        }


class SimulatedDisconnect(RuntimeError):
    def __init__(self, filename: str, resume_offset: int):
        super().__init__(f"simulated disconnect while streaming {filename} at offset {resume_offset}")
        self.filename = filename
        self.resume_offset = resume_offset


@dataclass
class FixtureRig:
    rig_info: RigInfo = field(
        default_factory=lambda: RigInfo(
            rig_serial="HS-PI5-MOCK-0001",
            fw="mock-r3-v0",
            model="pi5-bluez-fixture",
            hw_rev="mock-a",
        )
    )
    status: RigStatus = field(
        default_factory=lambda: RigStatus(
            uptime_s=3600,
            sd_free_mb=12453,
            battery=Battery(v_mv=3782, soc_pct=64, charging=True),
            last_detection_ts=1778252400,
            events_total=1247,
            calibration_status="metadata_only",
        )
    )
    files: tuple[ClipFile, ...] | None = None
    disconnect_after_chunks: int | None = None
    feedback_items: list[FeedbackItem] = field(default_factory=list)
    acked_deletes: set[str] = field(default_factory=set)

    def __post_init__(self):
        files = self.files if self.files is not None else _fixture_files()
        self.manifest = ClipManifest(self.rig_info, self.status, files)
        self._files = {clip.name: clip for clip in self.manifest.files}

    def device_info_bytes(self) -> bytes:
        return _stable_json_bytes(self.rig_info.to_dict())

    def status_bytes(self) -> bytes:
        return _stable_json_bytes(self.status.to_dict())

    def manifest_bytes(self, encoding: str = "cbor") -> bytes:
        if encoding == "json":
            return _stable_json_bytes(self.manifest.to_dict())
        if encoding == "cbor":
            return _cbor_encode(self.manifest.to_dict())
        raise ValueError(f"unsupported manifest encoding: {encoding}")

    def file_bytes(self, filename: str) -> bytes:
        return self._clip(filename).data

    def request_file(self, filename: str, offset: int = 0) -> Iterable[Chunk]:
        clip = self._clip(filename)
        if offset < 0 or offset > clip.size:
            raise ValueError(f"offset {offset} outside file {filename} size {clip.size}")
        chunks_sent = 0
        cursor = offset
        while cursor < clip.size:
            next_cursor = min(cursor + CHUNK_PAYLOAD_BYTES, clip.size)
            data = clip.data[cursor:next_cursor]
            yield Chunk(
                filename=filename,
                offset=cursor,
                data=data,
                eof=next_cursor == clip.size,
                file_crc32=clip.crc32_hex,
                chunk_crc16=_crc16_ccitt(data),
            )
            chunks_sent += 1
            cursor = next_cursor
            if self.disconnect_after_chunks is not None and chunks_sent >= self.disconnect_after_chunks:
                raise SimulatedDisconnect(filename, cursor)

    def ack_delete(self, filename: str) -> bool:
        self._clip(filename)
        self.acked_deletes.add(filename)
        return True

    def capture_feedback(self, payload: dict) -> FeedbackItem:
        filename = payload.get("file")
        self._clip(filename)
        verdict = payload.get("verdict")
        true_class = payload.get("true_class")
        if verdict not in VALID_VERDICTS:
            raise ValueError(f"invalid feedback verdict: {verdict}")
        if true_class not in VALID_TRUE_CLASSES:
            raise ValueError(f"invalid feedback true_class: {true_class}")
        item = FeedbackItem(
            file=filename,
            verdict=verdict,
            true_class=true_class,
            notes=payload.get("notes", ""),
            ts_review=payload.get("ts_review"),
        )
        self.feedback_items.append(item)
        return item

    def feedback_json_bytes(self) -> bytes:
        return _stable_json_bytes({"feedback": [item.to_dict() for item in self.feedback_items]})

    def _clip(self, filename: str) -> ClipFile:
        if not isinstance(filename, str) or filename not in self._files:
            raise FileNotFoundError(filename)
        return self._files[filename]


def _stable_json_bytes(value: dict) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc & 0xFFFF


def _cbor_encode(value) -> bytes:
    if value is None:
        return b"\xf6"
    if value is False:
        return b"\xf4"
    if value is True:
        return b"\xf5"
    if isinstance(value, int):
        if value < 0:
            return _cbor_type_len(1, -1 - value)
        return _cbor_type_len(0, value)
    if isinstance(value, float):
        import struct

        return b"\xfb" + struct.pack(">d", value)
    if isinstance(value, str):
        raw = value.encode("utf-8")
        return _cbor_type_len(3, len(raw)) + raw
    if isinstance(value, (bytes, bytearray)):
        raw = bytes(value)
        return _cbor_type_len(2, len(raw)) + raw
    if isinstance(value, list):
        return _cbor_type_len(4, len(value)) + b"".join(_cbor_encode(item) for item in value)
    if isinstance(value, dict):
        items = sorted(value.items(), key=lambda item: str(item[0]))
        out = bytearray(_cbor_type_len(5, len(items)))
        for key, item_value in items:
            out.extend(_cbor_encode(str(key)))
            out.extend(_cbor_encode(item_value))
        return bytes(out)
    raise TypeError(f"unsupported CBOR value: {type(value).__name__}")


def _cbor_type_len(major: int, length: int) -> bytes:
    if length < 24:
        return bytes([(major << 5) | length])
    if length <= 0xFF:
        return bytes([(major << 5) | 24, length])
    if length <= 0xFFFF:
        return bytes([(major << 5) | 25, (length >> 8) & 0xFF, length & 0xFF])
    if length <= 0xFFFFFFFF:
        return bytes(
            [
                (major << 5) | 26,
                (length >> 24) & 0xFF,
                (length >> 16) & 0xFF,
                (length >> 8) & 0xFF,
                length & 0xFF,
            ]
        )
    raise ValueError(f"CBOR length too large: {length}")


def _fixture_payload(name: str, blocks: int) -> bytes:
    seed = f"Hornet Snapper R3 mock fixture: {name}\n".encode("ascii")
    return (seed * blocks)[: blocks * 64]


def _fixture_files() -> tuple[ClipFile, ...]:
    return (
        ClipFile(
            name="2026-05-08_10-15-30_evt0001.mp4",
            data=_fixture_payload("evt0001.mp4", 9),
            ts_unix=1778235330,
            primary_class="vespa_velutina",
            max_conf=0.91,
            n_detections=17,
            thumbnail_idx=8,
            bbox={"x": 0.42, "y": 0.31, "w": 0.18, "h": 0.16},
        ),
        ClipFile(
            name="2026-05-08_10-17-42_evt0002.thumb.jpg",
            data=_fixture_payload("evt0002.thumb.jpg", 5),
            ts_unix=1778235462,
            primary_class="apis_mellifera",
            max_conf=0.74,
            n_detections=3,
            thumbnail_idx=0,
            bbox={"x": 0.12, "y": 0.22, "w": 0.21, "h": 0.19},
        ),
    )


SCENARIO_SERIALS = {
    "default": "HS-PI5-MOCK-0001",
    "low-battery": "HS-PI5-MOCK-0002",
    "stale-calibration": "HS-PI5-MOCK-0003",
    "empty": "HS-PI5-MOCK-0004",
}


def fixture_fleet() -> tuple[FixtureRig, ...]:
    return (
        FixtureRig(),
        FixtureRig(
            rig_info=RigInfo(
                rig_serial="HS-PI5-MOCK-0002",
                fw="mock-r3-v0",
                model="pi5-bluez-fixture",
                hw_rev="mock-a",
            ),
            status=RigStatus(
                uptime_s=4120,
                sd_free_mb=12011,
                battery=Battery(v_mv=3510, soc_pct=12, charging=False),
                last_detection_ts=1778252100,
                events_total=1248,
                calibration_status="metadata_only",
            ),
        ),
        FixtureRig(
            rig_info=RigInfo(
                rig_serial="HS-PI5-MOCK-0003",
                fw="mock-r3-v0",
                model="pi5-bluez-fixture",
                hw_rev="mock-a",
            ),
            status=RigStatus(
                uptime_s=28900,
                sd_free_mb=11880,
                battery=Battery(v_mv=3765, soc_pct=58, charging=False),
                last_detection_ts=1778169600,
                events_total=1189,
                calibration_status="stale",
            ),
        ),
        FixtureRig(
            rig_info=RigInfo(
                rig_serial="HS-PI5-MOCK-0004",
                fw="mock-r3-v0",
                model="pi5-bluez-fixture",
                hw_rev="mock-a",
            ),
            status=RigStatus(
                uptime_s=720,
                sd_free_mb=12780,
                battery=Battery(v_mv=3810, soc_pct=72, charging=True),
                last_detection_ts=0,
                events_total=0,
                calibration_status="metadata_only",
            ),
            files=(),
        ),
    )


def find_fixture_rig(serial: str) -> FixtureRig:
    for rig in fixture_fleet():
        if rig.rig_info.rig_serial == serial:
            return rig
    raise ValueError(f"unknown fixture rig: {serial}")


def select_fixture_rig(serial: str | None = None, scenario: str = "default") -> FixtureRig:
    if serial:
        return find_fixture_rig(serial)
    try:
        return find_fixture_rig(SCENARIO_SERIALS[scenario])
    except KeyError:
        raise ValueError(f"unknown fixture scenario: {scenario}") from None
