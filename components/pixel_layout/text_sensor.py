import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_PIXEL_LAYOUT_ID, CONF_SCREEN_INDEX, PixelLayout, PixelLayoutScreenLabelTextSensor, PixelLayoutSdStatusTextSensor

DEPENDENCIES = ["pixel_layout"]

CONFIG_SCHEMA = cv.typed_schema(
    {
        "sd_status": text_sensor.text_sensor_schema(
            PixelLayoutSdStatusTextSensor,
            icon="mdi:sd",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "screen_label": text_sensor.text_sensor_schema(
            PixelLayoutScreenLabelTextSensor,
            icon="mdi:label",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        )
        .extend(
            {
                cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout),
                cv.Required(CONF_SCREEN_INDEX): cv.int_range(min=0, max=31),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="sd_status",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    if config[CONF_TYPE] == "screen_label":
        cg.add(var.set_screen_index(config[CONF_SCREEN_INDEX]))
