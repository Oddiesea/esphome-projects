import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    ENTITY_CATEGORY_CONFIG,
)

from . import CONF_PIXEL_LAYOUT_ID, PixelLayout, PixelLayoutScreenSelect, PixelLayoutTransitionSelect

DEPENDENCIES = ["pixel_layout"]

TRANSITION_OPTIONS = [
    "cut",
    "fade",
    "slide_left",
    "slide_right",
    "slide_up",
    "slide_down",
    "wipe_left",
    "wipe_right",
    "wipe_up",
    "wipe_down",
    "iris",
    "dissolve",
    "blinds",
]

CONFIG_SCHEMA = cv.typed_schema(
    {
        "screen": select.select_schema(
            PixelLayoutScreenSelect,
            icon="mdi:view-carousel",
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
        "transition": select.select_schema(
            PixelLayoutTransitionSelect,
            icon="mdi:transition",
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        .extend({cv.GenerateID(CONF_PIXEL_LAYOUT_ID): cv.use_id(PixelLayout)})
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type="screen",
    lower=True,
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PIXEL_LAYOUT_ID])
    if config[CONF_TYPE] == "transition":
        var = await select.new_select(config, options=TRANSITION_OPTIONS)
        await cg.register_component(var, config)
        cg.add(var.set_parent(parent))
        return

    from esphome.core import CORE

    options = list(config.get("options") or [])
    if not options:
        # Prefer ids registered by pixel_layout to_code when platforms load after parent.
        for meta in (CORE.data.get("pixel_layout") or {}).values():
            ids = meta.get("screen_ids") if isinstance(meta, dict) else None
            if ids:
                options = list(ids)
                break
    if not options:
        options = ["screen"]
    var = await select.new_select(config, options=options)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
