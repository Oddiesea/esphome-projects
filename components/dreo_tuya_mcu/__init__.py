import esphome.codegen as cg
from esphome.components import button, number, select, switch, time, uart
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@liamjones"]
DEPENDENCIES = ["uart", "time"]
AUTO_LOAD = ["switch", "select", "number", "sensor", "text_sensor", "button"]
MULTI_CONF = False

CONF_DREO_TUYA_MCU_ID = "dreo_tuya_mcu_id"

dreo_tuya_mcu_ns = cg.esphome_ns.namespace("dreo_tuya_mcu")
DreoTuyaMcuComponent = dreo_tuya_mcu_ns.class_("DreoTuyaMcuComponent", cg.Component, uart.UARTDevice)
DreoTuyaSwitch = dreo_tuya_mcu_ns.class_("DreoTuyaSwitch", switch.Switch, cg.Component)
DreoTuyaSelect = dreo_tuya_mcu_ns.class_("DreoTuyaSelect", select.Select, cg.Component)
DreoTuyaNumber = dreo_tuya_mcu_ns.class_("DreoTuyaNumber", number.Number, cg.Component)
DreoTuyaQueryButton = dreo_tuya_mcu_ns.class_("DreoTuyaQueryButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DreoTuyaMcuComponent),
            cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


def _final_validate(config):
    require = uart.final_validate_device_schema(
        "dreo_tuya_mcu",
        baud_rate=115200,
        require_tx=True,
        require_rx=True,
    )
    require(config)
    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))
