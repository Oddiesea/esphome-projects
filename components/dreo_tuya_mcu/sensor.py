import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv

from . import CONF_DREO_TUYA_MCU_ID, DreoTuyaMcuComponent

DEPENDENCIES = ["dreo_tuya_mcu"]

CONF_DATAPOINT = "datapoint"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DREO_TUYA_MCU_ID): cv.use_id(DreoTuyaMcuComponent),
        cv.Required(CONF_DATAPOINT): cv.int_range(min=1, max=255),
    }
).extend(sensor.sensor_schema())


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DREO_TUYA_MCU_ID])
    sens = await sensor.new_sensor(config)
    cg.add(parent.register_sensor(config[CONF_DATAPOINT], sens))
