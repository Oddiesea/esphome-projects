import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_MODE,
    CONF_TYPE,
    ENTITY_CATEGORY_CONFIG,
)

from . import CONF_HUB75_DMA_ID, Hub75DmaBrightness, Hub75DmaCompensation, Hub75DmaDisplay

DEPENDENCIES = ["display"]

CONF_INITIAL_VALUE = "initial_value"

CONFIG_SCHEMA = cv.typed_schema(
    {
        "brightness": number.number_schema(
            Hub75DmaBrightness,
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_HUB75_DMA_ID): cv.use_id(Hub75DmaDisplay),
                cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "compensation": number.number_schema(
            Hub75DmaCompensation,
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_HUB75_DMA_ID): cv.use_id(Hub75DmaDisplay),
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                cv.Optional(CONF_INITIAL_VALUE, default=1.0): cv.float_range(min=0.1, max=5.0),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="brightness",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HUB75_DMA_ID])
    if config[CONF_TYPE] == "compensation":
        var = await number.new_number(config, min_value=0.1, max_value=5.0, step=0.1)
        await cg.register_component(var, config)
        cg.add(var.set_parent(parent))
        cg.add(parent.set_adaptive_compensation(var))
        cg.add(var.publish_state(config[CONF_INITIAL_VALUE]))
        return

    var = await number.new_number(config, min_value=0, max_value=255, step=1)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.register_brightness(var))
