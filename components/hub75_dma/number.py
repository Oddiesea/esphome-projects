import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_MODE, ENTITY_CATEGORY_CONFIG

from . import CONF_HUB75_DMA_ID, Hub75DmaBrightness, Hub75DmaDisplay

DEPENDENCIES = ["display"]

CONFIG_SCHEMA = (
    number.number_schema(
        Hub75DmaBrightness,
        entity_category=ENTITY_CATEGORY_CONFIG,
    )
    .extend(
        {
            cv.GenerateID(CONF_HUB75_DMA_ID): cv.use_id(Hub75DmaDisplay),
            cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HUB75_DMA_ID])
    var = await number.new_number(config, min_value=0, max_value=255, step=1)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.register_brightness(var))
