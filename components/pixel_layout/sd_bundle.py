"""Build uncompressed SD layout bundles for pixel_layout (directory tree + .plbundle).

Wire format (.plbundle):
  magic b'PLB1', u16 version (=1), u16 file_count,
  repeat: u16 path_len, path utf-8, u32 data_len, data bytes
"""

from __future__ import annotations

import base64
import hashlib
import io
import json
import re
import struct
import zipfile
from datetime import datetime, timezone
from typing import Any

import yaml

PLB_MAGIC = b"PLB1"
PLB_VERSION = 1
PLI_MAGIC = b"PLI1"
MANIFEST_NAME = "manifest.json"
PLAYLIST_NAME = "playlist.yml"
PLAYLIST_JSON_NAME = "playlist.json"

from pathlib import Path

_MATERIAL_SYMBOLS = json.loads(Path(__file__).with_name("material_symbols.json").read_text()).get("icons", {})

_ICONS = {
    "thermometer": "device_thermostat",
    "partlycloudy": "partly_cloudy_day",
    "clear-night": "clear_night",
}


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _ms(value: Any) -> int:
    if value is None:
        return 0
    if hasattr(value, "total_milliseconds"):
        return int(value.total_milliseconds)
    if isinstance(value, (int, float)):
        return int(value)
    if isinstance(value, str):
        s = value.strip().lower()
        if s.endswith("ms"):
            return int(float(s[:-2]))
        if s.endswith("s"):
            return int(float(s[:-1]) * 1000)
        return int(float(s))
    return 0


def _resolve_icon(value: str) -> str:
    key = value.strip()
    if key.lower().startswith("mdi:"):
        key = key[4:]
    key = key.replace("-", "_").replace(" ", "_")
    alias = _ICONS.get(key) or _ICONS.get(value)
    if alias:
        key = alias
    cp = _MATERIAL_SYMBOLS.get(key)
    if cp is not None:
        return chr(int(cp))
    return value


def _parse_color(value: Any) -> list[int] | str:
    if isinstance(value, list) and len(value) >= 3:
        return [int(value[0]), int(value[1]), int(value[2])]
    if not isinstance(value, str):
        return "white"
    s = value.strip()
    if s.lower() in ("white", "black"):
        return s.lower()
    if s.startswith("#") and len(s) >= 7:
        return [int(s[1:3], 16), int(s[3:5], 16), int(s[5:7], 16)]
    return s


def _pack_custom_pixels(value: Any) -> list[int] | None:
    if isinstance(value, str) and value.strip().startswith("p4:"):
        raw = base64.b64decode(value.strip()[3:])
        if len(raw) < 3:
            return None
        width, height = raw[1], raw[2]
        return [width, height, *raw[3:]]
    return None


def _normalize_widget(node: Any) -> Any:
    if not isinstance(node, dict):
        return node
    out = dict(node)
    t = out.get("type")
    if t == "icon" and isinstance(out.get("icon"), str):
        out["icon"] = _resolve_icon(out["icon"])
    for key in ("icon", "icon_end"):
        if isinstance(out.get(key), str):
            out[key] = _resolve_icon(out[key])
    if "color" in out:
        out["color"] = _parse_color(out["color"])
    if "fill" in out:
        out["fill"] = _parse_color(out["fill"])
    if "palette" in out and isinstance(out["palette"], list):
        out["palette"] = [_parse_color(c) for c in out["palette"]]
    if t == "custom":
        packed = _pack_custom_pixels(out.get("pixels"))
        if packed is not None:
            out["pixels_packed"] = packed
            out.pop("pixels", None)
    if "duration" in out:
        out["duration_ms"] = _ms(out.pop("duration"))
    if "transition_duration" in out:
        out["transition_ms"] = _ms(out.pop("transition_duration"))
    for key in ("children",):
        if isinstance(out.get(key), list):
            out[key] = [_normalize_widget(ch) for ch in out[key]]
    if isinstance(out.get("child"), dict):
        out["child"] = _normalize_widget(out["child"])
    return out


def normalize_playlist_json(playlist: dict[str, Any]) -> dict[str, Any]:
    """Device-facing playlist.json (ms numbers, resolved icons, packed pixels)."""
    out: dict[str, Any] = {}
    for key in ("display_id", "background", "font", "icon_font", "loop", "random"):
        if key in playlist:
            out[key] = playlist[key]
    if "background" in out and not isinstance(out["background"], list):
        out["background"] = _parse_color(out["background"])
    if "rotate" in playlist:
        out["rotate_ms"] = _ms(playlist["rotate"])
    if playlist.get("transition"):
        out["transition"] = str(playlist["transition"])
    if playlist.get("transition_duration"):
        out["transition_ms"] = _ms(playlist["transition_duration"])
    screens = []
    for i, screen in enumerate(playlist.get("screens") or []):
        if not isinstance(screen, dict):
            continue
        row: dict[str, Any] = {
            "id": screen.get("id") or f"screen_{i + 1}",
            "duration_ms": _ms(screen.get("duration") or playlist.get("rotate") or "8s"),
        }
        if screen.get("transition"):
            row["transition"] = str(screen["transition"])
        if screen.get("transition_duration"):
            row["transition_ms"] = _ms(screen["transition_duration"])
        if isinstance(screen.get("root"), dict):
            row["root"] = _normalize_widget(screen["root"])
        screens.append(row)
    out["screens"] = screens
    return out


def playlist_from_pixel_layout(doc: dict[str, Any]) -> dict[str, Any]:
    """Extract the pixel_layout root mapping from a device or layout doc."""
    if not isinstance(doc, dict):
        raise ValueError("layout doc must be a mapping")
    if "pixel_layout" in doc and isinstance(doc["pixel_layout"], dict):
        return dict(doc["pixel_layout"])
    if doc.get("screens") or doc.get("display_id"):
        return dict(doc)
    raise ValueError("doc has no pixel_layout section")


def _collect_sprite_ids(node: Any, out: set[str]) -> None:
    if isinstance(node, dict):
        if node.get("type") == "sprite":
            iid = node.get("image_id") or node.get("animation_id")
            if iid:
                out.add(str(iid))
        for key in ("children", "child", "root"):
            if key in node:
                _collect_sprite_ids(node[key], out)
        if node.get("type") == "stack" and "children" in node:
            for ch in node["children"] or []:
                _collect_sprite_ids(ch, out)
    elif isinstance(node, list):
        for item in node:
            _collect_sprite_ids(item, out)


def collect_pack_ids(playlist: dict[str, Any]) -> list[str]:
    ids: set[str] = set()
    screens = playlist.get("screens") or []
    if "root" in playlist:
        _collect_sprite_ids(playlist["root"], ids)
    for screen in screens:
        if isinstance(screen, dict):
            _collect_sprite_ids(screen.get("root"), ids)
    return sorted(ids)


def _rgb565_bin(pack_doc: dict[str, Any]) -> bytes:
    try:
        from .sprite_pack import _encode_rgb565, parse_sprite_pack
    except ImportError:
        from sprite_pack import _encode_rgb565, parse_sprite_pack
    pack = parse_sprite_pack(pack_doc, "pack")
    data, width, height = _encode_rgb565(pack["png"], pack["chroma_key"])
    chroma = 1 if pack["chroma_key"] else 0
    return PLI_MAGIC + struct.pack("<HHB", width, height, chroma) + bytes(data)


def build_sd_tree(
    layout_doc: dict[str, Any],
    *,
    panel_width: int = 128,
    panel_height: int = 64,
    packs: dict[str, dict[str, Any]] | None = None,
    icons: dict[str, dict[str, Any]] | None = None,
) -> dict[str, bytes]:
    """Return path → file bytes for an SD card layout folder."""
    playlist = playlist_from_pixel_layout(layout_doc)
    playlist_yaml = yaml.safe_dump(playlist, sort_keys=False, width=88).encode("utf-8")
    playlist_json_obj = normalize_playlist_json(playlist)
    playlist_json = (json.dumps(playlist_json_obj, separators=(",", ":")) + "\n").encode("utf-8")
    manifest = {
        "schema": 1,
        "created_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "panel_width": int(panel_width),
        "panel_height": int(panel_height),
        "playlist_sha256": _sha256(playlist_yaml),
        "playlist_json_sha256": _sha256(playlist_json),
        "pack_format": "sprite_v1_yaml",
    }
    tree: dict[str, bytes] = {
        MANIFEST_NAME: json.dumps(manifest, indent=2).encode("utf-8") + b"\n",
        PLAYLIST_NAME: playlist_yaml,
        PLAYLIST_JSON_NAME: playlist_json,
    }
    packs = packs or {}
    for pack_id, pack_doc in packs.items():
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in str(pack_id))
        tree[f"packs/{safe}.yml"] = yaml.safe_dump(pack_doc, sort_keys=False, width=88).encode("utf-8")
        try:
            tree[f"packs/{safe}.rgb565"] = _rgb565_bin(pack_doc)
        except Exception:
            pass
    icons = icons or {}
    for theme_id, icon_doc in icons.items():
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in str(theme_id))
        tree[f"icons/{safe}.yml"] = yaml.safe_dump(icon_doc, sort_keys=False, width=88).encode("utf-8")
    return tree


def build_plbundle(tree: dict[str, bytes]) -> bytes:
    """Serialise an uncompressed file tree into a .plbundle blob."""
    items = sorted(tree.items(), key=lambda kv: kv[0])
    buf = io.BytesIO()
    buf.write(PLB_MAGIC)
    buf.write(struct.pack("<HH", PLB_VERSION, len(items)))
    for path, data in items:
        path_b = path.encode("utf-8")
        if len(path_b) > 65535:
            raise ValueError(f"path too long: {path}")
        buf.write(struct.pack("<H", len(path_b)))
        buf.write(path_b)
        buf.write(struct.pack("<I", len(data)))
        buf.write(data)
    return buf.getvalue()


def parse_plbundle(raw: bytes) -> dict[str, bytes]:
    """Parse .plbundle bytes into path → contents."""
    if len(raw) < 8 or raw[:4] != PLB_MAGIC:
        raise ValueError("invalid plbundle magic")
    version, count = struct.unpack("<HH", raw[4:8])
    if version != PLB_VERSION:
        raise ValueError(f"unsupported plbundle version {version}")
    pos = 8
    out: dict[str, bytes] = {}
    for _ in range(count):
        if pos + 2 > len(raw):
            raise ValueError("truncated plbundle path_len")
        (path_len,) = struct.unpack("<H", raw[pos : pos + 2])
        pos += 2
        if pos + path_len + 4 > len(raw):
            raise ValueError("truncated plbundle path")
        path = raw[pos : pos + path_len].decode("utf-8")
        pos += path_len
        (data_len,) = struct.unpack("<I", raw[pos : pos + 4])
        pos += 4
        if pos + data_len > len(raw):
            raise ValueError("truncated plbundle data")
        out[path] = raw[pos : pos + data_len]
        pos += data_len
    if pos != len(raw):
        raise ValueError("trailing bytes in plbundle")
    return out


def export_sd_zip(tree: dict[str, bytes]) -> bytes:
    """Zip archive with ZIP_STORED (no compression) for manual SD card copy."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", compression=zipfile.ZIP_STORED) as zf:
        for path, data in sorted(tree.items()):
            zf.writestr(path, data)
    return buf.getvalue()


def pack_layout(
    layout_doc: dict[str, Any],
    *,
    panel_width: int = 128,
    panel_height: int = 64,
    packs: dict[str, dict[str, Any]] | None = None,
    icons: dict[str, dict[str, Any]] | None = None,
) -> tuple[dict[str, bytes], bytes]:
    """Build SD tree and .plbundle from a layout document."""
    tree = build_sd_tree(
        layout_doc,
        panel_width=panel_width,
        panel_height=panel_height,
        packs=packs,
        icons=icons,
    )
    return tree, build_plbundle(tree)
