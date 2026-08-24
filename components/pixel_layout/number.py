import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_MODE,
    CONF_TYPE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_SECOND,
)

from . import (
    CONF_PIXEL_LAYOUT_ID,
    PixelLayout,
    PixelLayoutNightHourNumber,
    PixelLayoutRotateIntervalNumber,
    PixelLayoutTransitionDurationNumber,
)

DEPENDENCIES = ["pixel_layout"]

_NIGHT_WHICH = {
    "off_hour": 0,
    "off_minute": 1,
    "on_hour": 2,
    "on_minute": 3,
}

CONFIG_SCHEMA = cv.typed_schema(
    {
        "rotate_interval": number.number_schema(
            PixelLayoutRotateIntervalNumber,
            unit_of_measurement=UNIT_SECOND,
            icon="mdi:timer",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "transition_duration": number.number_schema(
            PixelLayoutTransitionDurationNumber,
            unit_of_measurement=UNIT_SECOND,
            icon="mdi:timer-outline",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "off_hour": number.number_schema(
            PixelLayoutNightHourNumber,
            icon="mdi:clock-start",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "off_minute": number.number_schema(
            PixelLayoutNightHourNumber,
            icon="mdi:clock-start",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "on_hour": number.number_schema(
            PixelLayoutNightHourNumber,
            icon="mdi:clock-end",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "on_minute": number.number_schema(
            PixelLayoutNightHourNumber,
            icon="mdi:clock-end",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="rotate_interval",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    t = config[CONF_TYPE]
    if t == "transition_duration":
        var = await number.new_number(config, min_value=0.0, max_value=5.0, step=0.05)
        await cg.register_component(var, config)
        cg.add(var.set_parent(parent))
        return
    if t in _NIGHT_WHICH:
        max_v = 23.0 if t.endswith("hour") else 59.0
        var = await number.new_number(config, min_value=0.0, max_value=max_v, step=1.0)
        await cg.register_component(var, config)
        cg.add(var.set_parent(parent))
        cg.add(var.set_which(_NIGHT_WHICH[t]))
        return

    var = await number.new_number(config, min_value=1.0, max_value=600.0, step=1.0)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
