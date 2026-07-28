import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv

from . import CONF_DREO_TUYA_MCU_ID, DreoTuyaMcuComponent

DEPENDENCIES = ["dreo_tuya_mcu"]

CONF_DATAPOINT = "datapoint"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DREO_TUYA_MCU_ID): cv.use_id(DreoTuyaMcuComponent),
        cv.Required(CONF_DATAPOINT): cv.int_range(min=1, max=255),
    }
).extend(text_sensor.text_sensor_schema())


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DREO_TUYA_MCU_ID])
    ts = await text_sensor.new_text_sensor(config)
    cg.add(parent.register_text_sensor(config[CONF_DATAPOINT], ts))
