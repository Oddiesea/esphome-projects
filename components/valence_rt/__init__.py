import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@liamjones"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]
MULTI_CONF = False

CONF_VALENCE_RT_ID = "valence_rt_id"
CONF_MAX_BATTERIES = "max_batteries"
CONF_ROLE = "role"
CONF_LINK_UART_ID = "link_uart_id"

ROLE_DIRECT = "direct"
ROLE_CLIENT = "client"
ROLE_SERVER = "server"

valence_rt_ns = cg.esphome_ns.namespace("valence_rt")
ValenceRTComponent = valence_rt_ns.class_(
    "ValenceRTComponent", cg.PollingComponent, uart.UARTDevice
)
ValenceRole = valence_rt_ns.enum("ValenceRole", is_class=True)

ROLE_ENUM = {
    ROLE_DIRECT: ValenceRole.DIRECT,
    ROLE_CLIENT: ValenceRole.CLIENT,
    ROLE_SERVER: ValenceRole.SERVER,
}


def _validate_role(config):
    if config[CONF_ROLE] == ROLE_SERVER and CONF_LINK_UART_ID not in config:
        raise cv.Invalid("role: server requires link_uart_id (legacy bridge mode)")
    if config[CONF_ROLE] == ROLE_DIRECT and CONF_LINK_UART_ID in config:
        raise cv.Invalid("role: direct must not set link_uart_id")
    if config[CONF_ROLE] == ROLE_CLIENT and CONF_LINK_UART_ID in config:
        raise cv.Invalid("role: client must not set link_uart_id (uart_id is the link)")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ValenceRTComponent),
            cv.Optional(CONF_MAX_BATTERIES, default=1): cv.int_range(min=1, max=4),
            cv.Optional(CONF_ROLE, default=ROLE_DIRECT): cv.enum(ROLE_ENUM, lower=True),
            cv.Optional(CONF_LINK_UART_ID): cv.use_id(uart.UARTComponent),
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA),
    _validate_role,
)


def _final_validate(config):
    require = uart.final_validate_device_schema(
        "valence_rt",
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
    cg.add(var.set_max_batteries(config[CONF_MAX_BATTERIES]))
    cg.add(var.set_role(config[CONF_ROLE]))
    if CONF_LINK_UART_ID in config:
        link = await cg.get_variable(config[CONF_LINK_UART_ID])
        cg.add(var.set_link_uart(link))
