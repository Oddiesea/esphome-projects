import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    ENTITY_CATEGORY_CONFIG,
)

from . import (
    CONF_PIXEL_LAYOUT_ID,
    PixelLayout,
    PixelLayoutNightScheduleSwitch,
    PixelLayoutPinSwitch,
    PixelLayoutRandomSwitch,
    PixelLayoutRotateOverrideSwitch,
    PixelLayoutScreenEnabledSwitch,
    PixelLayoutTransitionOverrideSwitch,
    PixelLayoutUseSdLayoutSwitch,
)

DEPENDENCIES = ["pixel_layout"]

CONF_SCREEN_INDEX = "screen_index"

CONFIG_SCHEMA = cv.typed_schema(
    {
        "pin": switch.switch_schema(
            PixelLayoutPinSwitch,
            icon="mdi:pin",
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "random": switch.switch_schema(
            PixelLayoutRandomSwitch,
            icon="mdi:shuffle-variant",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "override_rotate": switch.switch_schema(
            PixelLayoutRotateOverrideSwitch,
            icon="mdi:timer-cog",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "override_transition": switch.switch_schema(
            PixelLayoutTransitionOverrideSwitch,
            icon="mdi:transition",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "screen_enabled": switch.switch_schema(
            PixelLayoutScreenEnabledSwitch,
            icon="mdi:eye",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Required(CONF_SCREEN_INDEX): cv.int_range(min=0, max=31),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "night_schedule": switch.switch_schema(
            PixelLayoutNightScheduleSwitch,
            icon="mdi:weather-night",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "use_sd_layout": switch.switch_schema(
            PixelLayoutUseSdLayoutSwitch,
            icon="mdi:sd",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="pin",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    t = config[CONF_TYPE]
    if t == "screen_enabled":
        cg.add(var.set_screen_index(config[CONF_SCREEN_INDEX]))
