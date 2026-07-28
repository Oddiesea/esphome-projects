import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_OPTIONS

from . import CONF_DREO_TUYA_MCU_ID, DreoTuyaMcuComponent, DreoTuyaSelect

DEPENDENCIES = ["dreo_tuya_mcu"]

CONF_DATAPOINT = "datapoint"

CONFIG_SCHEMA = (
    select.select_schema(DreoTuyaSelect)
    .extend(
        {
            cv.GenerateID(CONF_DREO_TUYA_MCU_ID): cv.use_id(DreoTuyaMcuComponent),
            cv.Required(CONF_DATAPOINT): cv.int_range(min=1, max=255),
            cv.Required(CONF_OPTIONS): cv.ensure_list(cv.string_strict),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DREO_TUYA_MCU_ID])
    var = await select.new_select(config, options=config[CONF_OPTIONS])
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_datapoint(config[CONF_DATAPOINT]))
    cg.add(parent.register_select(config[CONF_DATAPOINT], var))
