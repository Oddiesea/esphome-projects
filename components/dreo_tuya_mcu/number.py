import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_STEP

from . import CONF_DREO_TUYA_MCU_ID, DreoTuyaMcuComponent, DreoTuyaNumber

DEPENDENCIES = ["dreo_tuya_mcu"]

CONF_DATAPOINT = "datapoint"

CONFIG_SCHEMA = (
    number.number_schema(DreoTuyaNumber)
    .extend(
        {
            cv.GenerateID(CONF_DREO_TUYA_MCU_ID): cv.use_id(DreoTuyaMcuComponent),
            cv.Required(CONF_DATAPOINT): cv.int_range(min=1, max=255),
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Required(CONF_STEP): cv.positive_float,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DREO_TUYA_MCU_ID])
    var = await number.new_number(
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_datapoint(config[CONF_DATAPOINT]))
    cg.add(parent.register_number(config[CONF_DATAPOINT], var))
