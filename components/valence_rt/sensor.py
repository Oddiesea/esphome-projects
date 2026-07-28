import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_POWER,
    CONF_TEMPERATURE,
    CONF_VOLTAGE,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import CONF_VALENCE_RT_ID, ValenceRTComponent

DEPENDENCIES = ["valence_rt"]

CONF_BATTERY = "battery"
CONF_SOC = "soc"
CONF_CURRENT = "current"
CONF_CURRENT_2 = "current_2"
CONF_TEMPERATURE_PCB = "temperature_pcb"
CONF_CELL_VOLTAGE_1 = "cell_voltage_1"
CONF_CELL_VOLTAGE_2 = "cell_voltage_2"
CONF_CELL_VOLTAGE_3 = "cell_voltage_3"
CONF_CELL_VOLTAGE_4 = "cell_voltage_4"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_VALENCE_RT_ID): cv.use_id(ValenceRTComponent),
        cv.Required(CONF_BATTERY): cv.int_range(min=1, max=4),
        cv.Optional(CONF_SOC): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT_2): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE_PCB): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_1): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_2): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_3): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_4): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_VALENCE_RT_ID])
    index = config[CONF_BATTERY] - 1

    if CONF_SOC in config:
        sens = await sensor.new_sensor(config[CONF_SOC])
        cg.add(paren.set_soc_sensor(index, sens))
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(paren.set_voltage_sensor(index, sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(paren.set_current_sensor(index, sens))
    if CONF_CURRENT_2 in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_2])
        cg.add(paren.set_current_2_sensor(index, sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(paren.set_power_sensor(index, sens))
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(paren.set_temperature_sensor(index, sens))
    if CONF_TEMPERATURE_PCB in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_PCB])
        cg.add(paren.set_temperature_pcb_sensor(index, sens))
    if CONF_CELL_VOLTAGE_1 in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_1])
        cg.add(paren.set_cell_voltage_1_sensor(index, sens))
    if CONF_CELL_VOLTAGE_2 in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_2])
        cg.add(paren.set_cell_voltage_2_sensor(index, sens))
    if CONF_CELL_VOLTAGE_3 in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_3])
        cg.add(paren.set_cell_voltage_3_sensor(index, sens))
    if CONF_CELL_VOLTAGE_4 in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_4])
        cg.add(paren.set_cell_voltage_4_sensor(index, sens))
