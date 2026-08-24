from __future__ import annotations

import base64
import json
from pathlib import Path

from esphome import codegen as cg
from esphome.components import display, font, sensor, text_sensor, time
from esphome.components.color import ColorStruct
import esphome.config_validation as cv
from esphome.const import (
    CONF_COLOR,
    CONF_DELAY,
    CONF_DISPLAY_ID,
    CONF_DURATION,
    CONF_FONT,
    CONF_FORMAT,
    CONF_HEIGHT,
    CONF_ICON,
    CONF_ID,
    CONF_REPEAT,
    CONF_SENSOR_ID,
    CONF_TIME_ID,
    CONF_TYPE,
    CONF_WIDTH,
    CONF_X,
    CONF_Y,
)
from esphome.core import CORE, HexInt
from esphome.cpp_generator import RawExpression, RawStatement
from esphome.types import ConfigType

from .sprite_pack import validate_pack

CODEOWNERS = ["@liamjones"]
DEPENDENCIES = ["display"]
AUTO_LOAD = ["image"]

CONF_ROOT = "root"
CONF_SCREENS = "screens"
CONF_ROTATE = "rotate"
CONF_TRANSITION = "transition"
CONF_TRANSITION_DURATION = "transition_duration"
CONF_BACKGROUND = "background"
CONF_ICON_FONT = "icon_font"
CONF_CHILDREN = "children"
CONF_CHILD = "child"
CONF_GAP = "gap"
CONF_MAIN_ALIGN = "main_align"
CONF_CROSS_ALIGN = "cross_align"
CONF_EXPANDED = "expanded"
CONF_OPACITY = "opacity"
CONF_ANIMATION = "animation"
CONF_FACE = "face"
CONF_THEME = "theme"
CONF_BLINK_COLON = "blink_colon"
CONF_COLON = "colon"
CONF_SHOW_SECONDS = "show_seconds"
CONF_OUTLINE = "outline"
CONF_GHOST = "ghost"
CONF_SECONDARY_COLOR = "secondary_color"
CONF_ICON_COLOR = "icon_color"
CONF_ICON_END = "icon_end"
CONF_ICON_ALIGN = "icon_align"
CONF_ICON_GAP = "icon_gap"
CONF_TEXT_ALIGN = "text_align"
CONF_TEXT_SENSOR_ID = "text_sensor_id"
CONF_UNIT = "unit"
CONF_FILL = "fill"
CONF_RADIUS = "radius"
CONF_ANTIALIAS = "antialias"
CONF_KIND = "kind"
CONF_POINT = "point"
CONF_STROKE = "stroke"
CONF_PADDING = "padding"
CONF_IMAGE_ID = "image_id"
CONF_ANIMATION_ID = "animation_id"
CONF_PACK = "pack"
CONF_FRAME_WIDTH = "frame_width"
CONF_FRAME_HEIGHT = "frame_height"
CONF_FRAMES = "frames"
CONF_FPS = "fps"
CONF_LOOP = "loop"
CONF_STYLE = "style"
CONF_LABEL = "label"
CONF_UPPERCASE = "uppercase"
CONF_SHOW_YEAR = "show_year"
CONF_SIZE = "size"
CONF_CONDITION = "condition"
CONF_CONDITION_ID = "condition_id"
CONF_SHOW_ICON = "show_icon"
CONF_SHOW_CONDITION = "show_condition"
CONF_SHOW_TEMP = "show_temp"
CONF_SHOW_HUMIDITY = "show_humidity"
CONF_SHOW_WIND = "show_wind"
CONF_TEMPERATURE_ID = "temperature_id"
CONF_HUMIDITY_ID = "humidity_id"
CONF_WIND_SPEED_ID = "wind_speed_id"
CONF_WIND_BEARING_ID = "wind_bearing_id"
CONF_TEXT_POSITION = "text_position"
CONF_ICON_THEME = "icon_theme"
CONF_ICONS = "icons"
CONF_PIXELS = "pixels"
CONF_PALETTE = "palette"
CONF_VISIBLE = "visible"
CONF_ABOVE = "above"
CONF_BELOW = "below"
CONF_EQUAL = "equal"
CONF_NOT_EQUAL = "not_equal"
CONF_AT_LEAST = "at_least"
CONF_AT_MOST = "at_most"
CONF_STATE = "state"
CONF_INVERT = "invert"
CONF_AND = "and"
CONF_OR = "or"
CONF_FROM = "from"
CONF_TO = "to"
CONF_DX = "dx"
CONF_DY = "dy"
CONF_DIRECTION = "direction"
CONF_MODE = "mode"
PIXELS_P4 = "p4:"


def _pixel_index(ch: str) -> int:
    if not ch or ch in ". -0":
        return 0
    if ch in "Xx1":
        return 1
    if "2" <= ch <= "9":
        return int(ch)
    return 1


def _pack_pixel_indices(indices: list[int]) -> bytes:
    out = bytearray((len(indices) + 1) // 2)
    for i, idx in enumerate(indices):
        nibble = max(0, min(15, int(idx))) & 0x0F
        if i & 1:
            out[i >> 1] |= nibble
        else:
            out[i >> 1] = nibble << 4
    return bytes(out)


def validate_pixels(value):
    if isinstance(value, str):
        s = value.strip()
        if not s.startswith(PIXELS_P4):
            raise cv.Invalid("pixels as a string must be a p4: packed bitmap")
        try:
            raw = base64.b64decode(s[len(PIXELS_P4) :])
        except Exception as err:
            raise cv.Invalid(f"pixels: invalid p4 bitmap ({err})") from err
        if len(raw) < 3 or raw[0] != 1:
            raise cv.Invalid("pixels: unsupported or truncated p4 bitmap")
        width, height = raw[1], raw[2]
        if not 1 <= width <= 64 or not 1 <= height <= 64:
            raise cv.Invalid("pixels: p4 size must be 1–64")
        need = 3 + (width * height + 1) // 2
        if len(raw) < need:
            raise cv.Invalid("pixels: p4 bitmap too short")
        return s
    return cv.ensure_list(cv.string)(value)


def _pixels_packed(config: ConfigType) -> tuple[bytes, int, int] | None:
    raw = config.get(CONF_PIXELS)
    width = int(config[CONF_WIDTH]) if CONF_WIDTH in config else 0
    height = int(config[CONF_HEIGHT]) if CONF_HEIGHT in config else 0
    indices: list[int] = []
    if isinstance(raw, str) and raw.strip().startswith(PIXELS_P4):
        blob = base64.b64decode(raw.strip()[len(PIXELS_P4) :])
        width = blob[1]
        height = blob[2]
        count = width * height
        data = blob[3:]
        for i in range(count):
            if (i >> 1) >= len(data):
                indices.append(0)
                continue
            b = data[i >> 1]
            indices.append((b >> 4) if (i & 1) == 0 else (b & 0x0F))
    else:
        rows = list(raw or [])
        if not height:
            height = len(rows) or 0
        if not width:
            width = max((len(row) for row in rows), default=0)
        if width <= 0 or height <= 0:
            return None
        for y in range(height):
            row = rows[y] if y < len(rows) else ""
            for x in range(width):
                indices.append(_pixel_index(row[x] if x < len(row) else "."))
    if not indices or not any(indices):
        return None
    return _pack_pixel_indices(indices), width, height


async def _codegen_weather_icons(var, config: ConfigType) -> None:
    from .weather_theme import merge_weather_icons, normalize_weather_key

    icons = merge_weather_icons(config)
    if not icons:
        return
    widget_color = config.get(CONF_COLOR)
    for key, spec in icons.items():
        if not isinstance(spec, dict):
            continue
        icon_cfg = dict(spec)
        if CONF_COLOR not in icon_cfg and widget_color is not None:
            icon_cfg[CONF_COLOR] = widget_color
        if CONF_WIDTH not in icon_cfg:
            icon_cfg[CONF_WIDTH] = 16
        if CONF_HEIGHT not in icon_cfg:
            icon_cfg[CONF_HEIGHT] = 16
        packed = _pixels_packed(icon_cfg)
        if packed is None:
            continue
        data, width, height = packed
        n = CORE.data.setdefault("pixel_layout_bitmap_n", 0)
        CORE.data["pixel_layout_bitmap_n"] = n + 1
        name = f"pixel_layout_bitmap_{n}"
        body = ", ".join(str(HexInt(b)) for b in data)
        cg.add_global(RawStatement(f"static const uint8_t {name}[] PROGMEM = {{{body}}};"))
        if CONF_COLOR not in icon_cfg:
            icon_cfg[CONF_COLOR] = "white"
        color = _color_expr(validate_color(icon_cfg[CONF_COLOR]))
        palette = [validate_color(c) for c in (icon_cfg.get(CONF_PALETTE) or [])]
        if palette:
            pn = CORE.data.setdefault("pixel_layout_weather_pal_n", 0)
            CORE.data["pixel_layout_weather_pal_n"] = pn + 1
            pal_name = f"pixel_layout_weather_pal_{pn}"
            pal_body = ", ".join(str(_color_expr(p)) for p in palette)
            cg.add_global(RawStatement(f"static const Color {pal_name}[] PROGMEM = {{{pal_body}}};"))
            cg.add(
                var.add_custom_icon(
                    normalize_weather_key(key),
                    RawExpression(name),
                    width,
                    height,
                    color,
                    RawExpression(pal_name),
                    len(palette),
                )
            )
        else:
            cg.add(
                var.add_custom_icon(
                    normalize_weather_key(key),
                    RawExpression(name),
                    width,
                    height,
                    color,
                    RawExpression("nullptr"),
                    0,
                )
            )


pixel_layout_ns = cg.esphome_ns.namespace("pixel_layout")
PixelLayout = pixel_layout_ns.class_("PixelLayout", cg.Component)
Widget = pixel_layout_ns.class_("Widget")
StackWidget = pixel_layout_ns.class_("StackWidget", Widget)
RowWidget = pixel_layout_ns.class_("RowWidget", Widget)
ColumnWidget = pixel_layout_ns.class_("ColumnWidget", Widget)
BoxWidget = pixel_layout_ns.class_("BoxWidget", Widget)
TextWidget = pixel_layout_ns.class_("TextWidget", Widget)
IconWidget = pixel_layout_ns.class_("IconWidget", TextWidget)
ClockWidget = pixel_layout_ns.class_("ClockWidget", Widget)
DateWidget = pixel_layout_ns.class_("DateWidget", Widget)
WeatherWidget = pixel_layout_ns.class_("WeatherWidget", Widget)
SpriteWidget = pixel_layout_ns.class_("SpriteWidget", Widget)
CustomWidget = pixel_layout_ns.class_("CustomWidget", Widget)

AlignMain = pixel_layout_ns.enum("AlignMain", is_class=True)
AlignCross = pixel_layout_ns.enum("AlignCross", is_class=True)
ClockFace = pixel_layout_ns.enum("ClockFace", is_class=True)
ClockTheme = pixel_layout_ns.enum("ClockTheme", is_class=True)
ClockSize = pixel_layout_ns.enum("ClockSize", is_class=True)
Outline = pixel_layout_ns.enum("Outline", is_class=True)
IconAlign = pixel_layout_ns.enum("IconAlign", is_class=True)
DateStyle = pixel_layout_ns.enum("DateStyle", is_class=True)
TextStyle = pixel_layout_ns.enum("TextStyle", is_class=True)
WeatherTextPosition = pixel_layout_ns.enum("WeatherTextPosition", is_class=True)
BoxShape = pixel_layout_ns.enum("BoxShape", is_class=True)
BoxPoint = pixel_layout_ns.enum("BoxPoint", is_class=True)
AnimType = pixel_layout_ns.enum("AnimType", is_class=True)
AnimMode = pixel_layout_ns.enum("AnimMode", is_class=True)
ScreenTransition = pixel_layout_ns.enum("ScreenTransition", is_class=True)

MAIN_ALIGN = {
    "start": AlignMain.START,
    "center": AlignMain.CENTER,
    "end": AlignMain.END,
    "space_between": AlignMain.SPACE_BETWEEN,
}
CROSS_ALIGN = {
    "start": AlignCross.START,
    "center": AlignCross.CENTER,
    "end": AlignCross.END,
}
FACES = {"digital": ClockFace.DIGITAL, "analog": ClockFace.ANALOG}
THEMES = {
    "seven_segment": ClockTheme.SEVEN_SEGMENT,
    "rounded": ClockTheme.ROUNDED,
    "block": ClockTheme.BLOCK,
    "tiny": ClockTheme.TINY,
    "typeface": ClockTheme.TYPEFACE,
    "split_flap": ClockTheme.SPLIT_FLAP,
    "perspective": ClockTheme.PERSPECTIVE,
    "ring": ClockTheme.RING,
    "minimal": ClockTheme.MINIMAL,
    "ticks": ClockTheme.TICKS,
    "square": ClockTheme.SQUARE,
}
ANALOG_THEMES = {ClockTheme.RING, ClockTheme.MINIMAL, ClockTheme.TICKS, ClockTheme.SQUARE}
CLOCK_SIZES = {
    "sm": ClockSize.SM,
    "small": ClockSize.SM,
    "md": ClockSize.MD,
    "medium": ClockSize.MD,
    "lg": ClockSize.LG,
    "large": ClockSize.LG,
}
COLON_MODES = ("blink", "solid", "off")
OUTLINES = {
    "none": Outline.NONE,
    "off": Outline.NONE,
    "black": Outline.BLACK,
    "white": Outline.WHITE,
}
ICON_ALIGNS = {
    "top": IconAlign.TOP,
    "middle": IconAlign.MIDDLE,
    "center": IconAlign.MIDDLE,
    "bottom": IconAlign.BOTTOM,
}
DATE_STYLES = {
    "text": DateStyle.TEXT,
    "two_line": DateStyle.TWO_LINE,
    "calendar": DateStyle.CALENDAR,
}
WEATHER_TEXT_POSITIONS = {
    "end": WeatherTextPosition.END,
    "start": WeatherTextPosition.START,
    "below": WeatherTextPosition.BELOW,
    "above": WeatherTextPosition.ABOVE,
}
TEXT_STYLES = {
    "text": TextStyle.TEXT,
    "two_line": TextStyle.TWO_LINE,
}
BOX_SHAPES = {
    "rect": BoxShape.RECT,
    "rounded": BoxShape.ROUNDED,
    "oval": BoxShape.OVAL,
    "pill": BoxShape.PILL,
    "triangle": BoxShape.TRIANGLE,
    "diamond": BoxShape.DIAMOND,
    "plus": BoxShape.PLUS,
    "frame": BoxShape.FRAME,
    "ring": BoxShape.RING,
    "line": BoxShape.LINE,
}
BOX_POINTS = {
    "up": BoxPoint.UP,
    "down": BoxPoint.DOWN,
    "left": BoxPoint.LEFT,
    "right": BoxPoint.RIGHT,
}
ANIM_TYPES = {
    "fade": AnimType.FADE,
    "slide": AnimType.SLIDE,
    "pulse": AnimType.PULSE,
    "blink": AnimType.BLINK,
}
ANIM_MODES = {
    "in": AnimMode.IN,
    "out": AnimMode.OUT,
    "in_out": AnimMode.IN_OUT,
    "inout": AnimMode.IN_OUT,
    "both": AnimMode.IN_OUT,
}
DIRECTIONS = {
    "left": (-8, 0),
    "right": (8, 0),
    "up": (0, -8),
    "down": (0, 8),
}
PALETTE = {
    "white": (255, 255, 255),
    "warm": (255, 241, 194),
    "yellow": (255, 212, 0),
    "amber": (255, 176, 0),
    "orange": (255, 136, 0),
    "red": (255, 0, 0),
    "pink": (255, 107, 157),
    "magenta": (255, 0, 170),
    "purple": (170, 68, 255),
    "blue": (0, 0, 255),
    "navy": (51, 85, 204),
    "cyan": (0, 200, 255),
    "teal": (0, 200, 160),
    "green": (0, 255, 0),
    "lime": (180, 255, 0),
    "mint": (0, 255, 154),
    "brown": (139, 90, 43),
    "gray": (160, 160, 160),
    "grey": (160, 160, 160),
    "dim": (160, 160, 160),
    "charcoal": (85, 85, 85),
    "black": (0, 0, 0),
}
ICONS = {
    "schedule": "\uefd6",
    "thermometer": "\uf076",
    "water_drop": "\ue798",
    "home": "\ue9b2",
    "wifi": "\ue63e",
    "sunny": "\ue81a",
    "clear-night": "\uf159",
    "cloudy": "\uf15c",
    "partlycloudy": "\uf172",
    "rainy": "\uf176",
    "pouring": "\uf61f",
    "snowy": "\ue2cd",
    "hail": "\uf67f",
    "lightning": "\uebdb",
    "fog": "\ue818",
    "windy": "\uefd8",
    "exceptional": "\uf8b6",
}
_SYMBOLS_PATH = Path(__file__).with_name("material_symbols.json")
MATERIAL_SYMBOLS = json.loads(_SYMBOLS_PATH.read_text()).get("icons", {}) if _SYMBOLS_PATH.is_file() else {}
WEATHER_CONDITIONS = (
    "sunny",
    "clear",
    "clear-night",
    "cloudy",
    "overcast",
    "partlycloudy",
    "rainy",
    "pouring",
    "snowy",
    "snowy-rainy",
    "hail",
    "lightning",
    "lightning-rainy",
    "fog",
    "windy",
    "windy-variant",
    "exceptional",
)

_THEMES_JSON = json.loads(Path(__file__).with_name("themes.json").read_text())


def validate_color(value):
    if isinstance(value, int):
        return {"rgb": ((value >> 16) & 255, (value >> 8) & 255, value & 255), "a": 255}
    if isinstance(value, str):
        raw = value.strip()
        key = raw.lower()
        if key in PALETTE:
            return {"rgb": PALETTE[key], "a": 255}
        if raw.startswith("#"):
            h = raw[1:]
            if len(h) == 6:
                return {
                    "rgb": (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)),
                    "a": 255,
                }
            if len(h) == 8:
                return {
                    "rgb": (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)),
                    "a": int(h[6:8], 16),
                }
            raise cv.Invalid("hex color must be #RRGGBB or #RRGGBBAA")
        if raw.startswith("0x") or raw.startswith("0X"):
            n = int(raw, 16)
            return {"rgb": ((n >> 16) & 255, (n >> 8) & 255, n & 255), "a": 255}
        return {"id": cv.use_id(ColorStruct)(raw)}
    raise cv.Invalid("color must be hex, palette name, or color id")


def validate_icon(value):
    value = cv.string(value)
    if value in ICONS:
        return ICONS[value]
    key = value.strip()
    if key.lower().startswith("mdi:"):
        key = key[4:]
    key = key.replace("-", "_").replace(" ", "_")
    if key in ICONS:
        return ICONS[key]
    cp = MATERIAL_SYMBOLS.get(key)
    if cp is not None:
        return chr(int(cp))
    raise cv.Invalid(
        f"Unknown icon '{value}'. Use a Material Symbols name (home, thermostat) "
        "or an alias such as thermometer, partlycloudy."
    )


def validate_outline(value):
    if isinstance(value, bool):
        return Outline.BLACK if value else Outline.NONE
    if isinstance(value, int):
        return Outline.BLACK if value else Outline.NONE
    return cv.enum(OUTLINES, lower=True)(value)


SIDE_ICON_SCHEMA = {
    cv.Optional(CONF_ICON): validate_icon,
    cv.Optional(CONF_ICON_END): validate_icon,
    cv.Optional(CONF_ICON_COLOR): validate_color,
    cv.Optional(CONF_ICON_ALIGN): cv.enum(ICON_ALIGNS, lower=True),
    cv.Optional(CONF_TEXT_ALIGN, default="middle"): cv.enum(ICON_ALIGNS, lower=True),
    cv.Optional(CONF_ICON_GAP): cv.int_range(min=0, max=8),
}


ANIM_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TYPE): cv.enum(ANIM_TYPES, lower=True),
        cv.Optional(CONF_MODE, default="in"): cv.enum(ANIM_MODES, lower=True),
        cv.Optional(CONF_DURATION, default="400ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_DELAY, default="0ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_REPEAT, default=0): cv.int_range(min=-1, max=1000),
        cv.Optional(CONF_FROM, default=0): cv.int_range(min=0, max=255),
        cv.Optional(CONF_TO, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_DX, default=0): cv.int_range(min=-1024, max=1024),
        cv.Optional(CONF_DY, default=0): cv.int_range(min=-1024, max=1024),
        cv.Optional(CONF_DIRECTION): cv.enum(DIRECTIONS, lower=True),
    }
)


_VISIBLE_SENSOR_OPS = (
    CONF_ABOVE,
    CONF_BELOW,
    CONF_EQUAL,
    CONF_NOT_EQUAL,
    CONF_AT_LEAST,
    CONF_AT_MOST,
)


def _validate_visible_leaf(config):
    config = dict(config)
    if "equals" in config:
        if CONF_EQUAL in config:
            raise cv.Invalid("visible cannot set both equal: and equals:")
        config[CONF_EQUAL] = config.pop("equals")
    has_sensor = CONF_SENSOR_ID in config
    has_text = CONF_TEXT_SENSOR_ID in config
    if has_sensor == has_text:
        raise cv.Invalid("visible condition requires exactly one of sensor_id or text_sensor_id")
    ops = [k for k in _VISIBLE_SENSOR_OPS if k in config]
    if has_sensor:
        if CONF_STATE in config:
            raise cv.Invalid("sensor_id uses a numeric comparison, not state")
        if not ops:
            raise cv.Invalid("sensor_id requires above, below, equal, not_equal, at_least, or at_most")
        exclusive = {CONF_EQUAL, CONF_NOT_EQUAL}
        if exclusive & set(ops) and len(ops) > 1:
            raise cv.Invalid("equal/not_equal cannot mix with other comparisons")
        range_ok = set(ops) <= {CONF_ABOVE, CONF_BELOW} or set(ops) <= {CONF_AT_LEAST, CONF_AT_MOST}
        if len(ops) > 1 and not range_ok:
            raise cv.Invalid("combine only above+below or at_least+at_most on one condition")
    if has_text:
        if ops:
            raise cv.Invalid("text_sensor_id uses state, not numeric comparisons")
        if CONF_STATE not in config:
            raise cv.Invalid("text_sensor_id requires state")
    return config


VISIBLE_LEAF = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_TEXT_SENSOR_ID): cv.use_id(text_sensor.TextSensor),
            cv.Optional(CONF_ABOVE): cv.float_,
            cv.Optional(CONF_BELOW): cv.float_,
            cv.Optional(CONF_EQUAL): cv.float_,
            cv.Optional("equals"): cv.float_,
            cv.Optional(CONF_NOT_EQUAL): cv.float_,
            cv.Optional(CONF_AT_LEAST): cv.float_,
            cv.Optional(CONF_AT_MOST): cv.float_,
            cv.Optional(CONF_STATE): cv.string,
            cv.Optional(CONF_INVERT, default=False): cv.boolean,
        }
    ),
    _validate_visible_leaf,
)


def validate_visible(value):
    if not isinstance(value, dict):
        raise cv.Invalid("visible must be a mapping")
    has_and = CONF_AND in value
    has_or = CONF_OR in value
    if has_and and has_or:
        raise cv.Invalid("visible cannot set both and: and or:")
    if has_and or has_or:
        extra = set(value) - {CONF_AND, CONF_OR, CONF_INVERT}
        if extra:
            raise cv.Invalid(f"visible and/or cannot mix with {', '.join(sorted(extra))}")
        raw = value.get(CONF_AND) if has_and else value.get(CONF_OR)
        items = cv.ensure_list(VISIBLE_LEAF)(raw)
        if not items:
            raise cv.Invalid("visible and/or requires at least one condition")
        out = {"match": "all" if has_and else "any", "when": items}
        if value.get(CONF_INVERT):
            out[CONF_INVERT] = True
        return out
    return {"match": "all", "when": [VISIBLE_LEAF(value)]}


VISIBLE_SCHEMA = validate_visible


def _base(cls):
    return {
        cv.GenerateID(): cv.declare_id(cls),
        cv.Required(CONF_TYPE): cv.string,
        cv.Optional(CONF_X): cv.int_range(min=-512, max=512),
        cv.Optional(CONF_Y): cv.int_range(min=-512, max=512),
        cv.Optional(CONF_WIDTH): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_HEIGHT): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_OPACITY, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_EXPANDED, default=False): cv.boolean,
        cv.Optional(CONF_ANIMATION): ANIM_SCHEMA,
        cv.Optional(CONF_VISIBLE): VISIBLE_SCHEMA,
        cv.Optional(CONF_FONT): cv.use_id(font.Font),
        cv.Optional(CONF_ICON_FONT): cv.use_id(font.Font),
        cv.Optional(CONF_COLOR): validate_color,
        cv.Optional(CONF_OUTLINE, default="none"): validate_outline,
    }


def validate_widget(value):
    if not isinstance(value, dict):
        raise cv.Invalid("widget must be a mapping with type:")
    if CONF_TYPE not in value:
        raise cv.Invalid("widget requires type")
    t = cv.one_of(*WIDGET_SCHEMAS, lower=True)(value[CONF_TYPE])
    value = dict(value)
    value[CONF_TYPE] = t
    return WIDGET_SCHEMAS[t](value)


def _validate_clock(config):
    face = config[CONF_FACE]
    theme = config[CONF_THEME]
    if face == ClockFace.ANALOG:
        if theme not in ANALOG_THEMES:
            config[CONF_THEME] = ClockTheme.RING
    elif theme in ANALOG_THEMES:
        config[CONF_THEME] = ClockTheme.SEVEN_SEGMENT
    return config


WIDGET_SCHEMAS: dict = {}

WIDGET_SCHEMAS["stack"] = cv.Schema(
    {
        **_base(StackWidget),
        cv.Optional(CONF_CHILDREN, default=[]): cv.ensure_list(validate_widget),
    }
)
WIDGET_SCHEMAS["row"] = cv.Schema(
    {
        **_base(RowWidget),
        cv.Optional(CONF_CHILDREN, default=[]): cv.ensure_list(validate_widget),
        cv.Optional(CONF_GAP, default=0): cv.int_range(min=0, max=64),
        cv.Optional(CONF_MAIN_ALIGN, default="start"): cv.enum(MAIN_ALIGN, lower=True),
        cv.Optional(CONF_CROSS_ALIGN, default="center"): cv.enum(CROSS_ALIGN, lower=True),
    }
)
WIDGET_SCHEMAS["column"] = cv.Schema(
    {
        **_base(ColumnWidget),
        cv.Optional(CONF_CHILDREN, default=[]): cv.ensure_list(validate_widget),
        cv.Optional(CONF_GAP, default=0): cv.int_range(min=0, max=64),
        cv.Optional(CONF_MAIN_ALIGN, default="start"): cv.enum(MAIN_ALIGN, lower=True),
        cv.Optional(CONF_CROSS_ALIGN, default="start"): cv.enum(CROSS_ALIGN, lower=True),
    }
)
_BOX_SCHEMA = cv.Schema(
    {
        **_base(BoxWidget),
        cv.Optional(CONF_CHILD): validate_widget,
        cv.Optional(CONF_PADDING, default=0): cv.int_range(min=0, max=64),
        cv.Optional(CONF_FILL): validate_color,
        cv.Optional(CONF_COLOR): validate_color,
        cv.Optional(CONF_KIND, default="rect"): cv.enum(BOX_SHAPES, lower=True),
        cv.Optional(CONF_POINT, default="up"): cv.enum(BOX_POINTS, lower=True),
        cv.Optional(CONF_STROKE, default=1): cv.int_range(min=1, max=32),
        cv.Optional(CONF_RADIUS, default=0): cv.int_range(min=0, max=128),
        cv.Optional(CONF_ANTIALIAS, default=False): cv.boolean,
    }
)
WIDGET_SCHEMAS["box"] = _BOX_SCHEMA
WIDGET_SCHEMAS["shape"] = _BOX_SCHEMA
_TEXT_SCHEMA = cv.Schema(
    {
        **_base(TextWidget),
        cv.Optional("text", default=""): cv.string,
        cv.Optional(CONF_FORMAT): cv.string,
        cv.Optional(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_TEXT_SENSOR_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_UNIT, default=""): cv.string,
        cv.Optional(CONF_LABEL, default=""): cv.string,
        cv.Optional(CONF_STYLE, default="text"): cv.enum(TEXT_STYLES, lower=True),
        **SIDE_ICON_SCHEMA,
    }
)
WIDGET_SCHEMAS["text"] = _TEXT_SCHEMA
WIDGET_SCHEMAS["sensor"] = _TEXT_SCHEMA
WIDGET_SCHEMAS["icon"] = cv.Schema(
    {
        **_base(IconWidget),
        cv.Required(CONF_ICON): validate_icon,
    }
)
WIDGET_SCHEMAS["clock"] = cv.All(
    cv.Schema(
        {
            **_base(ClockWidget),
            cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Optional(CONF_FACE, default="digital"): cv.enum(FACES, lower=True),
            cv.Optional(CONF_THEME, default="seven_segment"): cv.enum(THEMES, lower=True),
            cv.Optional(CONF_SIZE, default="md"): cv.enum(CLOCK_SIZES, lower=True),
            cv.Optional(CONF_COLON): cv.one_of(*COLON_MODES, lower=True),
            cv.Optional(CONF_BLINK_COLON, default=True): cv.boolean,
            cv.Optional(CONF_SHOW_SECONDS, default=False): cv.boolean,
            cv.Optional(CONF_GHOST, default=False): cv.boolean,
            cv.Optional(CONF_SECONDARY_COLOR): validate_color,
            cv.Optional(CONF_FORMAT): cv.string,
            **SIDE_ICON_SCHEMA,
        }
    ),
    _validate_clock,
)
WIDGET_SCHEMAS["date"] = cv.Schema(
    {
        **_base(DateWidget),
        cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Optional(CONF_STYLE, default="text"): cv.enum(DATE_STYLES, lower=True),
        cv.Optional(CONF_FORMAT, default="%a %d %b"): cv.string,
        cv.Optional(CONF_UPPERCASE, default=False): cv.boolean,
        cv.Optional(CONF_SHOW_YEAR, default=False): cv.boolean,
        **SIDE_ICON_SCHEMA,
    }
)
WIDGET_SCHEMAS["weather"] = cv.All(
    cv.Schema(
        {
            **_base(WeatherWidget),
            cv.Optional(CONF_CONDITION_ID): cv.use_id(text_sensor.TextSensor),
            cv.Optional(CONF_CONDITION): cv.one_of(*WEATHER_CONDITIONS, lower=True),
            cv.Optional(CONF_ICON_THEME): cv.string,
            cv.Optional(CONF_ICONS): cv.schema(
                {
                    cv.string: cv.Schema(
                        {
                            cv.Optional(CONF_COLOR): validate_color,
                            cv.Optional(CONF_PALETTE, default=[]): cv.ensure_list(validate_color),
                            cv.Required(CONF_PIXELS): validate_pixels,
                            cv.Optional(CONF_WIDTH): cv.int_range(min=1, max=64),
                            cv.Optional(CONF_HEIGHT): cv.int_range(min=1, max=64),
                        }
                    )
                }
            ),
            cv.Optional(CONF_SHOW_ICON, default=True): cv.boolean,
            cv.Optional(CONF_SHOW_CONDITION, default=False): cv.boolean,
            cv.Optional(CONF_SHOW_TEMP, default=False): cv.boolean,
            cv.Optional(CONF_SHOW_HUMIDITY, default=False): cv.boolean,
            cv.Optional(CONF_SHOW_WIND, default=False): cv.boolean,
            cv.Optional(CONF_TEXT_POSITION, default="end"): cv.enum(WEATHER_TEXT_POSITIONS, lower=True),
            cv.Optional(CONF_ICON_ALIGN, default="middle"): cv.enum(ICON_ALIGNS, lower=True),
            cv.Optional(CONF_TEXT_ALIGN, default="middle"): cv.enum(ICON_ALIGNS, lower=True),
            cv.Optional(CONF_GAP, default=2): cv.int_range(min=0, max=16),
            cv.Optional(CONF_TEMPERATURE_ID): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_HUMIDITY_ID): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_WIND_SPEED_ID): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_WIND_BEARING_ID): cv.use_id(sensor.Sensor),
        }
    ),
    cv.has_at_least_one_key(CONF_CONDITION_ID, CONF_CONDITION),
)

WEATHER_ICON_THEME = cv.string


def _validate_icon_theme(config):
    if CONF_ICON_THEME not in config:
        return config
    from .weather_theme import list_weather_themes

    theme = str(config[CONF_ICON_THEME])
    names = list_weather_themes()
    if theme not in names and not (theme == "mario" and "gameman" in names):
        raise cv.Invalid(f"Unknown weather icon theme '{theme}'")
    return config


WIDGET_SCHEMAS["weather"] = cv.All(
    WIDGET_SCHEMAS["weather"],
    _validate_icon_theme,
)
WIDGET_SCHEMAS["sprite"] = cv.Schema(
    {
        **_base(SpriteWidget),
        cv.Optional(CONF_PACK): validate_pack,
        cv.Optional(CONF_IMAGE_ID): cv.string,
        cv.Optional(CONF_ANIMATION_ID): cv.string,
        cv.Optional(CONF_FRAME_WIDTH): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_FRAME_HEIGHT): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_FRAMES): cv.int_range(min=1, max=256),
        cv.Optional(CONF_FPS): cv.float_range(min=0.1, max=60),
        cv.Optional(CONF_LOOP): cv.boolean,
    }
)
WIDGET_SCHEMAS["custom"] = cv.Schema(
    {
        **_base(CustomWidget),
        cv.Optional(CONF_PIXELS, default=[]): validate_pixels,
        cv.Optional(CONF_PALETTE, default=[]): cv.ensure_list(validate_color),
    }
)


def _validate_sprite_ids(config):
    if config[CONF_TYPE] != "sprite":
        return config
    if CONF_PACK not in config and CONF_IMAGE_ID not in config and CONF_ANIMATION_ID not in config:
        raise cv.Invalid("sprite requires pack, image_id, or animation_id")
    if CONF_FPS not in config:
        config[CONF_FPS] = 10
    if CONF_LOOP not in config:
        config[CONF_LOOP] = True
    return config


def _walk_validate_sprite(config):
    if isinstance(config, dict) and config.get(CONF_TYPE) == "sprite":
        if CONF_PACK in config:
            from .sprite_pack import apply_pack_to_widget, register_sprite_pack

            apply_pack_to_widget(config, register_sprite_pack(config[CONF_PACK]))
        _validate_sprite_ids(config)
        if CONF_IMAGE_ID in config:
            from esphome.core import ID
            from esphome.components.image import Image_

            if not isinstance(config[CONF_IMAGE_ID], ID):
                config[CONF_IMAGE_ID] = cv.use_id(Image_)(config[CONF_IMAGE_ID])
        if CONF_ANIMATION_ID in config:
            from esphome.components.animation.image import Animation_

            config[CONF_ANIMATION_ID] = cv.use_id(Animation_)(config[CONF_ANIMATION_ID])
    for key in (CONF_CHILDREN,):
        for child in config.get(key, []) if isinstance(config, dict) else []:
            _walk_validate_sprite(child)
    if isinstance(config, dict) and CONF_CHILD in config:
        _walk_validate_sprite(config[CONF_CHILD])
    if isinstance(config, dict) and CONF_ROOT in config:
        _walk_validate_sprite(config[CONF_ROOT])
    if isinstance(config, dict) and CONF_SCREENS in config:
        for screen in config.get(CONF_SCREENS) or []:
            if isinstance(screen, dict) and CONF_ROOT in screen:
                _walk_validate_sprite(screen[CONF_ROOT])
    return config


def _expand_screen(config):
    config = dict(config)
    if CONF_ROOT not in config:
        raise cv.Invalid("screen requires root:")
    config[CONF_ROOT] = validate_widget(config[CONF_ROOT])
    return config


SCREEN_TRANSITIONS = {
    "none": ScreenTransition.CUT,
    "cut": ScreenTransition.CUT,
    "fade": ScreenTransition.FADE,
    "slide": ScreenTransition.SLIDE_LEFT,
    "slide_left": ScreenTransition.SLIDE_LEFT,
    "slide_right": ScreenTransition.SLIDE_RIGHT,
    "slide_up": ScreenTransition.SLIDE_UP,
    "slide_down": ScreenTransition.SLIDE_DOWN,
    "wipe": ScreenTransition.WIPE_LEFT,
    "wipe_left": ScreenTransition.WIPE_LEFT,
    "wipe_right": ScreenTransition.WIPE_RIGHT,
    "wipe_up": ScreenTransition.WIPE_UP,
    "wipe_down": ScreenTransition.WIPE_DOWN,
    "iris": ScreenTransition.IRIS,
    "dissolve": ScreenTransition.DISSOLVE,
    "blinds": ScreenTransition.BLINDS,
}

SCREEN_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_ID): cv.string,
            cv.Optional(CONF_DURATION): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_TRANSITION): cv.enum(SCREEN_TRANSITIONS, lower=True),
            cv.Optional(CONF_TRANSITION_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_ROOT): dict,
        }
    ),
    _expand_screen,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PixelLayout),
            cv.Required(CONF_DISPLAY_ID): cv.use_id(display.Display),
            cv.Optional(CONF_ROOT): validate_widget,
            cv.Optional(CONF_SCREENS): cv.ensure_list(SCREEN_SCHEMA),
            cv.Optional(CONF_ROTATE, default="8s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_TRANSITION, default="fade"): cv.enum(SCREEN_TRANSITIONS, lower=True),
            cv.Optional(CONF_TRANSITION_DURATION, default="400ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_LOOP, default=True): cv.boolean,
            cv.Optional("random", default=False): cv.boolean,
            cv.Optional(CONF_FONT): cv.use_id(font.Font),
            cv.Optional(CONF_ICON_FONT): cv.use_id(font.Font),
            cv.Optional(CONF_BACKGROUND, default="black"): validate_color,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_ROOT, CONF_SCREENS),
    _walk_validate_sprite,
)


def _color_expr(spec: ConfigType):
    if "id" in spec:
        return None
    r, g, b = spec["rgb"]
    a = spec.get("a", 255)
    return RawExpression(f"Color({r}, {g}, {b}, {a})")


async def _apply_color(var, setter, spec):
    if spec is None:
        return
    if "id" in spec:
        col = await cg.get_variable(spec["id"])
        cg.add(getattr(var, setter)(col))
        return
    cg.add(getattr(var, setter)(_color_expr(spec)))


async def _apply_side_icons(var, config):
    if CONF_ICON in config:
        cg.add(var.set_icon(config[CONF_ICON]))
    if CONF_ICON_END in config:
        cg.add(var.set_icon_end(config[CONF_ICON_END]))
    if CONF_ICON_COLOR in config:
        await _apply_color(var, "set_icon_color", config[CONF_ICON_COLOR])
    if CONF_ICON_ALIGN in config:
        cg.add(var.set_icon_align(config[CONF_ICON_ALIGN]))
    if CONF_TEXT_ALIGN in config:
        cg.add(var.set_text_align(config[CONF_TEXT_ALIGN]))
    if CONF_ICON_GAP in config:
        cg.add(var.set_icon_gap(config[CONF_ICON_GAP]))


def _ms(value) -> int:
    if hasattr(value, "total_milliseconds"):
        return int(value.total_milliseconds)
    return int(value)


async def _apply_common(var, config, defaults):
    if CONF_X in config:
        cg.add(var.set_x(config[CONF_X]))
    if CONF_Y in config:
        cg.add(var.set_y(config[CONF_Y]))
    if CONF_WIDTH in config:
        cg.add(var.set_width(config[CONF_WIDTH]))
    if CONF_HEIGHT in config:
        cg.add(var.set_height(config[CONF_HEIGHT]))
    cg.add(var.set_opacity(config[CONF_OPACITY]))
    cg.add(var.set_expanded(config[CONF_EXPANDED]))
    cg.add(var.set_outline(config[CONF_OUTLINE]))
    t = config[CONF_TYPE]
    if t in ("text", "icon", "clock", "sensor", "date", "weather", "custom") and CONF_COLOR in config:
        await _apply_color(var, "set_color", config[CONF_COLOR])
    if t in ("text", "icon", "clock", "sensor", "date"):
        font_id = config.get(CONF_FONT, defaults.get(CONF_FONT))
        if font_id is not None:
            f = await cg.get_variable(font_id)
            cg.add(var.set_font(f))
        icon_font_id = config.get(CONF_ICON_FONT, defaults.get(CONF_ICON_FONT))
        if icon_font_id is not None and t in ("clock", "sensor", "text", "icon", "date"):
            f = await cg.get_variable(icon_font_id)
            if t == "icon":
                cg.add(var.set_font(f))
            else:
                cg.add(var.set_icon_font(f))
    if t == "weather":
        font_id = config.get(CONF_FONT, defaults.get(CONF_FONT))
        if font_id is not None:
            f = await cg.get_variable(font_id)
            cg.add(var.set_font(f))
        icon_font_id = config.get(CONF_ICON_FONT, defaults.get(CONF_ICON_FONT))
        if icon_font_id is not None:
            f = await cg.get_variable(icon_font_id)
            cg.add(var.set_icon_font(f))
    if CONF_ANIMATION in config:
        anim = config[CONF_ANIMATION]
        dx, dy = anim[CONF_DX], anim[CONF_DY]
        if CONF_DIRECTION in anim:
            dx, dy = anim[CONF_DIRECTION]
        cg.add(
            var.set_animation(
                anim[CONF_TYPE],
                _ms(anim[CONF_DURATION]),
                _ms(anim[CONF_DELAY]),
                anim[CONF_REPEAT],
                dx,
                dy,
                anim[CONF_FROM],
                anim[CONF_TO],
                anim[CONF_MODE],
            )
        )
    if CONF_VISIBLE in config:
        vis = config[CONF_VISIBLE]
        cg.add(var.set_visible_match_all(vis.get("match", "all") != "any"))
        if vis.get(CONF_INVERT):
            cg.add(var.set_visible_invert(True))
        for clause in vis.get("when") or []:
            invert = bool(clause.get(CONF_INVERT))
            if CONF_SENSOR_ID in clause:
                sens = await cg.get_variable(clause[CONF_SENSOR_ID])
                ops = []
                for key, code in (
                    (CONF_ABOVE, 1),
                    (CONF_AT_LEAST, 2),
                    (CONF_EQUAL, 3),
                    (CONF_NOT_EQUAL, 4),
                    (CONF_BELOW, 5),
                    (CONF_AT_MOST, 6),
                ):
                    if key in clause:
                        ops.append((code, clause[key]))
                cmp1, val1 = ops[0]
                cmp2, val2 = ops[1] if len(ops) > 1 else (0, 0)
                cg.add(var.add_visible_sensor(sens, cmp1, val1, cmp2, val2, invert))
            elif CONF_TEXT_SENSOR_ID in clause:
                ts = await cg.get_variable(clause[CONF_TEXT_SENSOR_ID])
                cg.add(var.add_visible_text(ts, clause[CONF_STATE], invert))


async def build_widget(config: ConfigType, defaults: dict):
    var = cg.new_Pvariable(config[CONF_ID])
    await _apply_common(var, config, defaults)
    t = config[CONF_TYPE]
    if t in ("stack", "row", "column"):
        if t != "stack":
            cg.add(var.set_gap(config[CONF_GAP]))
            cg.add(var.set_main_align(config[CONF_MAIN_ALIGN]))
            cg.add(var.set_cross_align(config[CONF_CROSS_ALIGN]))
        for child_conf in config.get(CONF_CHILDREN, []):
            child = await build_widget(child_conf, defaults)
            cg.add(var.add_child(child))
    elif t in ("box", "shape"):
        cg.add(var.set_padding(config[CONF_PADDING]))
        cg.add(var.set_shape(config[CONF_KIND]))
        cg.add(var.set_point(config[CONF_POINT]))
        cg.add(var.set_stroke(config[CONF_STROKE]))
        cg.add(var.set_radius(config[CONF_RADIUS]))
        cg.add(var.set_antialias(config[CONF_ANTIALIAS]))
        fill = config.get(CONF_FILL, config.get(CONF_COLOR))
        if fill is not None:
            await _apply_color(var, "set_fill", fill)
        if CONF_CHILD in config:
            child = await build_widget(config[CONF_CHILD], defaults)
            cg.add(var.set_child(child))
    elif t in ("text", "sensor"):
        body = config.get("text") or config.get(CONF_FORMAT) or ""
        cg.add(var.set_text(body))
        if CONF_FORMAT in config:
            cg.add(var.set_format(config[CONF_FORMAT]))
        if CONF_SENSOR_ID in config:
            sens = await cg.get_variable(config[CONF_SENSOR_ID])
            cg.add(var.set_sensor(sens))
        if CONF_TEXT_SENSOR_ID in config:
            ts = await cg.get_variable(config[CONF_TEXT_SENSOR_ID])
            cg.add(var.set_text_sensor(ts))
        if config.get(CONF_UNIT):
            cg.add(var.set_unit(config[CONF_UNIT]))
        if config.get(CONF_LABEL):
            cg.add(var.set_caption(config[CONF_LABEL]))
        cg.add(var.set_style(config[CONF_STYLE]))
        await _apply_side_icons(var, config)
    elif t == "icon":
        cg.add(var.set_codepoint(config[CONF_ICON]))
    elif t == "clock":
        rtc = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(rtc))
        cg.add(var.set_face(config[CONF_FACE]))
        cg.add(var.set_theme(config[CONF_THEME]))
        cg.add(var.set_size(config[CONF_SIZE]))
        if CONF_COLON in config:
            colon = config[CONF_COLON]
            cg.add(var.set_show_colon(colon != "off"))
            cg.add(var.set_blink_colon(colon == "blink"))
        else:
            cg.add(var.set_blink_colon(config[CONF_BLINK_COLON]))
            cg.add(var.set_show_colon(True))
        cg.add(var.set_show_seconds(config[CONF_SHOW_SECONDS]))
        cg.add(var.set_ghost(config[CONF_GHOST]))
        if CONF_SECONDARY_COLOR in config:
            await _apply_color(var, "set_secondary_color", config[CONF_SECONDARY_COLOR])
        if CONF_FORMAT in config:
            cg.add(var.set_format(config[CONF_FORMAT]))
        await _apply_side_icons(var, config)
    elif t == "date":
        rtc = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(rtc))
        cg.add(var.set_style(config[CONF_STYLE]))
        cg.add(var.set_format(config[CONF_FORMAT]))
        cg.add(var.set_uppercase(config[CONF_UPPERCASE]))
        cg.add(var.set_show_year(config[CONF_SHOW_YEAR]))
        await _apply_side_icons(var, config)
    elif t == "weather":
        if CONF_CONDITION_ID in config:
            ts = await cg.get_variable(config[CONF_CONDITION_ID])
            cg.add(var.set_condition_sensor(ts))
        if CONF_CONDITION in config:
            cg.add(var.set_condition(config[CONF_CONDITION]))
        cg.add(var.set_show_icon(config[CONF_SHOW_ICON]))
        cg.add(var.set_show_condition(config[CONF_SHOW_CONDITION]))
        cg.add(var.set_show_temp(config[CONF_SHOW_TEMP]))
        cg.add(var.set_show_humidity(config[CONF_SHOW_HUMIDITY]))
        cg.add(var.set_show_wind(config[CONF_SHOW_WIND]))
        cg.add(var.set_text_position(config[CONF_TEXT_POSITION]))
        cg.add(var.set_icon_align(config[CONF_ICON_ALIGN]))
        cg.add(var.set_text_align(config[CONF_TEXT_ALIGN]))
        cg.add(var.set_gap(config[CONF_GAP]))
        if CONF_TEMPERATURE_ID in config:
            sens = await cg.get_variable(config[CONF_TEMPERATURE_ID])
            cg.add(var.set_temperature_sensor(sens))
        if CONF_HUMIDITY_ID in config:
            sens = await cg.get_variable(config[CONF_HUMIDITY_ID])
            cg.add(var.set_humidity_sensor(sens))
        if CONF_WIND_SPEED_ID in config:
            sens = await cg.get_variable(config[CONF_WIND_SPEED_ID])
            cg.add(var.set_wind_speed_sensor(sens))
        if CONF_WIND_BEARING_ID in config:
            sens = await cg.get_variable(config[CONF_WIND_BEARING_ID])
            cg.add(var.set_wind_bearing_sensor(sens))
        await _codegen_weather_icons(var, config)
    elif t == "sprite":
        if CONF_IMAGE_ID in config:
            img = await cg.get_variable(config[CONF_IMAGE_ID])
            cg.add(var.set_image(img))
        if CONF_ANIMATION_ID in config:
            anim = await cg.get_variable(config[CONF_ANIMATION_ID])
            cg.add(var.set_gif(anim))
        if CONF_FRAME_WIDTH in config:
            cg.add(var.set_frame_width(config[CONF_FRAME_WIDTH]))
        if CONF_FRAME_HEIGHT in config:
            cg.add(var.set_frame_height(config[CONF_FRAME_HEIGHT]))
        if CONF_FRAMES in config:
            cg.add(var.set_frames(config[CONF_FRAMES]))
        cg.add(var.set_fps(config[CONF_FPS]))
        cg.add(var.set_loop(config[CONF_LOOP]))
    elif t == "custom":
        for spec in config.get(CONF_PALETTE) or []:
            await _apply_color(var, "add_palette_color", spec)
        packed = _pixels_packed(config)
        if packed is not None:
            data, width, height = packed
            n = CORE.data.setdefault("pixel_layout_bitmap_n", 0)
            CORE.data["pixel_layout_bitmap_n"] = n + 1
            name = f"pixel_layout_bitmap_{n}"
            body = ", ".join(str(HexInt(b)) for b in data)
            cg.add_global(RawStatement(f"static const uint8_t {name}[] PROGMEM = {{{body}}};"))
            cg.add(var.set_pixels(RawExpression(name), width, height))
    return var


async def to_code(config: ConfigType):
    from .sprite_pack import codegen_sprite_packs

    await codegen_sprite_packs()
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    disp = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_display(disp))
    await _apply_color(var, "set_background", config[CONF_BACKGROUND])
    defaults = {}
    if CONF_FONT in config:
        f = await cg.get_variable(config[CONF_FONT])
        cg.add(var.set_font(f))
        defaults[CONF_FONT] = config[CONF_FONT]
    if CONF_ICON_FONT in config:
        f = await cg.get_variable(config[CONF_ICON_FONT])
        cg.add(var.set_icon_font(f))
        defaults[CONF_ICON_FONT] = config[CONF_ICON_FONT]
    cg.add(var.set_rotate_ms(_ms(config[CONF_ROTATE])))
    cg.add(var.set_transition(config[CONF_TRANSITION]))
    cg.add(var.set_transition_ms(_ms(config[CONF_TRANSITION_DURATION])))
    cg.add(var.set_screen_loop(config[CONF_LOOP]))
    cg.add(var.set_screen_random(config["random"]))
    screens = config.get(CONF_SCREENS)
    if not screens:
        screens = [{CONF_ROOT: config[CONF_ROOT]}]
    for screen in screens:
        root = await build_widget(screen[CONF_ROOT], defaults)
        duration = _ms(screen[CONF_DURATION]) if CONF_DURATION in screen else 0
        trans = screen.get(CONF_TRANSITION, config[CONF_TRANSITION])
        trans_ms = (
            _ms(screen[CONF_TRANSITION_DURATION])
            if CONF_TRANSITION_DURATION in screen
            else _ms(config[CONF_TRANSITION_DURATION])
        )
        cg.add(var.add_screen(root, duration, trans, trans_ms))


# Keep JSON available for tests / docs.
THEMES_JSON = _THEMES_JSON
