import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_CONFIG,
    ICON_POWER,
)

from . import CONF_HUB75_DMA_ID, Hub75DmaAdaptiveSwitch, Hub75DmaDisplay, Hub75DmaPowerSwitch

DEPENDENCIES = ["display"]

CONFIG_SCHEMA = cv.typed_schema(
    {
        "power": switch.switch_schema(
            Hub75DmaPowerSwitch,
            device_class=DEVICE_CLASS_SWITCH,
            icon=ICON_POWER,
            default_restore_mode="ALWAYS_ON",
        )
        .extend(
            {
                cv.GenerateID(CONF_HUB75_DMA_ID): cv.use_id(Hub75DmaDisplay),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        "adaptive": switch.switch_schema(
            Hub75DmaAdaptiveSwitch,
            icon="mdi:brightness-auto",
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON",
        )
        .extend(
            {
                cv.GenerateID(CONF_HUB75_DMA_ID): cv.use_id(Hub75DmaDisplay),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="power",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HUB75_DMA_ID])
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    if config[CONF_TYPE] == "adaptive":
        cg.add(parent.set_adaptive_enable_switch(var))
    else:
        cg.add(parent.register_power_switch(var))
