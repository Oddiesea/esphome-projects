import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_TYPE

from . import CONF_PIXEL_LAYOUT_ID, PixelLayout, PixelLayoutNextButton, PixelLayoutReloadSdLayoutButton, PixelLayoutSleepButton

DEPENDENCIES = ["pixel_layout"]

CONFIG_SCHEMA = cv.typed_schema(
    {
        "next": button.button_schema(
            PixelLayoutNextButton,
            icon="mdi:skip-next",
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "sleep": button.button_schema(
            PixelLayoutSleepButton,
            icon="mdi:sleep",
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "reload_sd_layout": button.button_schema(
            PixelLayoutReloadSdLayoutButton,
            icon="mdi:reload",
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="next",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    var = await button.new_button(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
