import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv

from . import CONF_DREO_TUYA_MCU_ID, DreoTuyaMcuComponent, DreoTuyaQueryButton

DEPENDENCIES = ["dreo_tuya_mcu"]

CONFIG_SCHEMA = (
    button.button_schema(DreoTuyaQueryButton)
    .extend(
        {
            cv.GenerateID(CONF_DREO_TUYA_MCU_ID): cv.use_id(DreoTuyaMcuComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DREO_TUYA_MCU_ID])
    var = await button.new_button(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.register_query_button(var))
