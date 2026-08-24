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
    PixelLayoutRotateIntervalNumber,
    PixelLayoutTransitionDurationNumber,
)

DEPENDENCIES = ["pixel_layout"]

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
    },
    default_type="rotate_interval",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    if config[CONF_TYPE] == "transition_duration":
        var = await number.new_number(config, min_value=0.0, max_value=5.0, step=0.05)
        await cg.register_component(var, config)
        cg.add(var.set_parent(parent))
        return

    var = await number.new_number(config, min_value=1.0, max_value=600.0, step=1.0)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
