"""pixel_layout.sprite/v1 packs: YAML (or JSON) with PNG + frame grid + fps."""

from __future__ import annotations

import base64
import hashlib
import io
import json
import re
import struct
from pathlib import Path

import esphome.config_validation as cv
from esphome.core import CORE, HexInt
from esphome.types import ConfigType

FORMAT = "pixel_layout.sprite/v1"
FORMAT_ALIASES = {FORMAT, "pixel_layout.sprite.v1"}
PACK_KEY = "pixel_layout_sprite_packs"

_DATA_URL = re.compile(r"^data:image/[^;]+;base64,", re.I)


def cut_lines(width: int, height: int, frame_width: int, frame_height: int) -> dict:
    fw = max(1, int(frame_width))
    fh = max(1, int(frame_height))
    return {
        "x": list(range(fw, int(width), fw)),
        "y": list(range(fh, int(height), fh)),
    }


def _png_size(data: bytes) -> tuple[int, int]:
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        raise cv.Invalid("sprite pack png must be a PNG image")
    if data[12:16] != b"IHDR":
        raise cv.Invalid("sprite pack png is missing IHDR")
    width, height = struct.unpack(">II", data[16:24])
    if width < 1 or height < 1:
        raise cv.Invalid("sprite pack png has invalid size")
    return width, height


def _b64_png(value) -> bytes:
    if not value or not isinstance(value, str):
        raise cv.Invalid("sprite pack needs png as a base64 string")
    raw = _DATA_URL.sub("", value.strip())
    try:
        data = base64.b64decode(raw, validate=False)
    except (ValueError, TypeError) as exc:
        raise cv.Invalid("sprite pack png is not valid base64") from exc
    _png_size(data)
    return data


def _int(value, default, minimum=1, maximum=256):
    if value is None:
        return default
    try:
        n = int(value)
    except (TypeError, ValueError):
        return default
    return max(minimum, min(maximum, n))


def _float(value, default, minimum=0.1, maximum=60.0):
    if value is None:
        return default
    try:
        n = float(value)
    except (TypeError, ValueError):
        return default
    return max(minimum, min(maximum, n))


def parse_sprite_pack(data: dict, source: str = "pack") -> dict:
    if not isinstance(data, dict):
        raise cv.Invalid(f"{source} must be a YAML mapping")
    fmt = data.get("format")
    if fmt is not None and fmt not in FORMAT_ALIASES:
        raise cv.Invalid(f"{source} format must be {FORMAT}")
    png = _b64_png(data.get("png") or data.get("image"))
    width, height = _png_size(png)
    if width > 256 or height > 256:
        raise cv.Invalid("sprite pack PNG must be 256×256 or smaller")

    fw = _int(data.get("frame_width"), 0)
    fh = _int(data.get("frame_height"), 0)
    columns = _int(data.get("columns"), 0)
    rows = _int(data.get("rows"), 0)
    cuts = data.get("cuts") if isinstance(data.get("cuts"), dict) else {}
    cut_x = cuts.get("x") if isinstance(cuts.get("x"), list) else []
    cut_y = cuts.get("y") if isinstance(cuts.get("y"), list) else []
    if not fw and cut_x:
        try:
            fw = int(sorted(cut_x)[0])
        except (TypeError, ValueError, IndexError):
            fw = 0
    if not fh and cut_y:
        try:
            fh = int(sorted(cut_y)[0])
        except (TypeError, ValueError, IndexError):
            fh = 0
    if not fw and columns:
        fw = max(1, width // columns)
    if not fh and rows:
        fh = max(1, height // rows)
    if not fw:
        fw = width
    if not fh:
        fh = height
    columns = max(1, width // fw)
    rows = max(1, height // fh)
    capacity = columns * rows
    frames = _int(data.get("frames"), capacity, maximum=256)
    frames = max(1, min(frames, capacity))
    ident = str(data.get("id") or "").strip()
    if not ident:
        ident = "s" + hashlib.sha256(png).hexdigest()[:12]
    ident = cv.validate_id_name(ident)
    return {
        "id": ident,
        "name": str(data.get("name") or ident),
        "png": png,
        "width": width,
        "height": height,
        "frame_width": fw,
        "frame_height": fh,
        "columns": columns,
        "rows": rows,
        "cuts": cut_lines(width, height, fw, fh),
        "frames": frames,
        "fps": _float(data.get("fps"), 10),
        "loop": bool(data["loop"]) if "loop" in data else True,
        "chroma_key": bool(data["chroma_key"]) if "chroma_key" in data else True,
    }


def _load_mapping(text: str, source: str) -> dict:
    stripped = text.lstrip()
    if stripped.startswith("{") or stripped.startswith("["):
        try:
            data = json.loads(text)
        except json.JSONDecodeError as exc:
            raise cv.Invalid(f"{source} is not valid JSON") from exc
    else:
        import yaml

        try:
            data = yaml.safe_load(text)
        except yaml.YAMLError as exc:
            raise cv.Invalid(f"{source} is not valid YAML") from exc
    if isinstance(data, dict) and isinstance(data.get("pack"), dict) and "png" not in data:
        data = data["pack"]
    if not isinstance(data, dict):
        raise cv.Invalid(f"{source} must be a YAML mapping")
    return data


def load_sprite_pack_file(path: str | Path) -> dict:
    file_path = Path(path)
    if not file_path.is_file():
        raise cv.Invalid(f"sprite pack not found: {file_path}")
    return parse_sprite_pack(_load_mapping(file_path.read_text(encoding="utf-8"), str(file_path)), str(file_path))


def validate_pack(value):
    """File path, or an inline YAML mapping (including !include)."""
    if isinstance(value, dict):
        parse_sprite_pack(value, "pack")
        return value
    return cv.file_(value)


def register_sprite_pack(value) -> dict:
    if isinstance(value, dict):
        parsed = parse_sprite_pack(value, "pack")
    else:
        parsed = load_sprite_pack_file(value)
    store = CORE.data.setdefault(PACK_KEY, {})
    ident = parsed["id"]
    if ident in store:
        return store[ident]
    from esphome import codegen as cg
    from esphome.components.image import Image_

    cpp_id = f"pl_pack_{ident}"
    store[ident] = {
        **parsed,
        "image_id": cv.declare_id(Image_)(cpp_id),
        "raw_id": cv.declare_id(cg.uint8)(f"{cpp_id}_data"),
    }
    return store[ident]


def apply_pack_to_widget(config: ConfigType, pack: dict) -> None:
    config.setdefault("frame_width", pack["frame_width"])
    config.setdefault("frame_height", pack["frame_height"])
    config.setdefault("frames", pack["frames"])
    config.setdefault("fps", pack["fps"])
    config.setdefault("loop", pack["loop"])
    config["image_id"] = pack["image_id"]


def _encode_rgb565(png: bytes, chroma_key: bool) -> tuple[list[int], int, int]:
    from PIL import Image as PILImage

    try:
        from esphome.components.image import CONF_CHROMA_KEY, CONF_OPAQUE, IMAGE_TYPE

        image = PILImage.open(io.BytesIO(png))
        width, height = image.size
        transparency = CONF_CHROMA_KEY if chroma_key else CONF_OPAQUE
        encoder = IMAGE_TYPE["RGB565"](width, height, transparency, PILImage.Dither.NONE, False)
        encoder.set_big_endian(False)
        pixels = encoder.convert(image.resize((width, height)), "sprite_pack").getdata()
        for row in range(height):
            for col in range(width):
                encoder.encode(pixels[row * width + col])
            encoder.end_row()
        encoder.end_image()
        return encoder.data, width, height
    except (ImportError, KeyError, TypeError):
        image = PILImage.open(io.BytesIO(png)).convert("RGBA")
        width, height = image.size
        data: list[int] = []
        for r, g, b, a in image.getdata():
            r5, g6, b5 = r >> 3, g >> 2, b >> 3
            if chroma_key:
                if r5 == 0 and g6 == 1 and b5 == 0:
                    g6 = 0
                elif a < 128:
                    r5, g6, b5 = 0, 1, 0
            rgb = (r5 << 11) | (g6 << 5) | b5
            data.append(rgb & 0xFF)
            data.append(rgb >> 8)
        return data, width, height


async def codegen_sprite_packs() -> None:
    from esphome import codegen as cg
    from esphome.components.image import add_metadata, get_image_type_enum, get_transparency_enum

    cg.add_define("USE_IMAGE")
    for pack in CORE.data.get(PACK_KEY, {}).values():
        data, width, height = _encode_rgb565(pack["png"], pack["chroma_key"])
        prog_arr = cg.progmem_array(pack["raw_id"], [HexInt(x) for x in data])
        transparency = "chroma_key" if pack["chroma_key"] else "opaque"
        cg.new_Pvariable(
            pack["image_id"],
            prog_arr,
            width,
            height,
            get_image_type_enum("RGB565"),
            get_transparency_enum(transparency),
        )
        try:
            add_metadata(pack["image_id"], width, height, "RGB565", transparency)
        except Exception:
            pass
