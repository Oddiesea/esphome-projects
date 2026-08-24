import esphome.codegen as cg
from esphome.components import display, number, switch
from esphome.components.esp32 import add_idf_component, add_idf_sdkconfig_option
from esphome.core import CORE

CODEOWNERS = ["@liamjones"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["display", "number", "switch"]

CONF_HUB75_DMA_ID = "hub75_dma_id"

# Pinned release of https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
DMA_LIBRARY_GIT = "https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA.git"
DMA_LIBRARY_REF = "3.0.14"

hub75_dma_ns = cg.esphome_ns.namespace("hub75_dma")
Hub75DmaDisplay = hub75_dma_ns.class_(
    "Hub75DmaDisplay", cg.PollingComponent, display.DisplayBuffer
)
Hub75DmaBrightness = hub75_dma_ns.class_(
    "Hub75DmaBrightness", number.Number, cg.Component
)
Hub75DmaPowerSwitch = hub75_dma_ns.class_(
    "Hub75DmaPowerSwitch", switch.Switch, cg.Component
)


def add_dma_library():
    """Pull ESP32-HUB75-MatrixPanel-DMA into the firmware build without Adafruit GFX."""
    cg.add_build_flag("-DNO_GFX=1")
    if CORE.is_esp32 and not CORE.using_arduino:
        # Library Kconfig defaults this on, which then REQUIRES Arduino + Adafruit GFX.
        add_idf_sdkconfig_option("CONFIG_ESP32_HUB75_USE_GFX", False)
        add_idf_component(
            name="ESP32-HUB75-MatrixPanel-DMA",
            repo=DMA_LIBRARY_GIT,
            ref=DMA_LIBRARY_REF,
        )
    else:
        cg.add_library(
            name="ESP32-HUB75-MatrixPanel-DMA",
            repository=DMA_LIBRARY_GIT,
            version=DMA_LIBRARY_REF,
        )
