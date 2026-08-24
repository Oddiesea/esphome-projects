"""Build uncompressed SD layout bundles for pixel_layout (directory tree + .plbundle).

Wire format (.plbundle):
  magic b'PLB1', u16 version (=1), u16 file_count,
  repeat: u16 path_len, path utf-8, u32 data_len, data bytes
"""

from __future__ import annotations

import hashlib
import io
import json
import struct
import zipfile
from datetime import datetime, timezone
from typing import Any

import yaml

PLB_MAGIC = b"PLB1"
PLB_VERSION = 1
MANIFEST_NAME = "manifest.json"
PLAYLIST_NAME = "playlist.yml"


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


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
    manifest = {
        "schema": 1,
        "created_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "panel_width": int(panel_width),
        "panel_height": int(panel_height),
        "playlist_sha256": _sha256(playlist_yaml),
        "pack_format": "sprite_v1_yaml",
    }
    tree: dict[str, bytes] = {
        MANIFEST_NAME: json.dumps(manifest, indent=2).encode("utf-8") + b"\n",
        PLAYLIST_NAME: playlist_yaml,
    }
    packs = packs or {}
    for pack_id, pack_doc in packs.items():
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in str(pack_id))
        tree[f"packs/{safe}.yml"] = yaml.safe_dump(pack_doc, sort_keys=False, width=88).encode("utf-8")
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
