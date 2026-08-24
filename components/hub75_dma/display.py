from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import display, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_BOARD,
    CONF_BRIGHTNESS,
    CONF_CLK_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_OE_PIN,
    CONF_SENSOR_ID,
    CONF_UPDATE_INTERVAL,
)
from esphome.core import ID
from esphome.cpp_generator import MockObj, TemplateArgsType
from esphome.types import ConfigType

from . import Hub75DmaDisplay, add_dma_library, hub75_dma_ns
from .boards import (
    BOARD_BOUNDS,
    BOARDS,
    CONF_A_PIN,
    CONF_B_PIN,
    CONF_B1_PIN,
    CONF_B2_PIN,
    CONF_C_PIN,
    CONF_CLOCK_PHASE,
    CONF_D_PIN,
    CONF_E_PIN,
    CONF_G1_PIN,
    CONF_G2_PIN,
    CONF_LAT_PIN,
    CONF_R1_PIN,
    CONF_R2_PIN,
    PIN_KEYS,
    REQUIRED_PIN_KEYS,
)

DEPENDENCIES = ["esp32"]

CONF_PANEL_WIDTH = "panel_width"
CONF_PANEL_HEIGHT = "panel_height"
CONF_CHAIN_LENGTH = "chain_length"
CONF_CHAIN_ROWS = "chain_rows"
CONF_CHAIN_COLS = "chain_cols"
CONF_CHAIN_TYPE = "chain_type"
CONF_CHAIN_ROTATION = "chain_rotation"
CONF_DRIVER = "driver"
CONF_LINE_DECODER = "line_decoder"
CONF_I2SSPEED = "i2sspeed"
CONF_LATCH_BLANKING = "latch_blanking"
CONF_DOUBLE_BUFFER = "double_buffer"
CONF_USE_PSRAM = "use_psram"
CONF_ADAPTIVE_BRIGHTNESS = "adaptive_brightness"
CONF_MIN_BRIGHTNESS = "min_brightness"
CONF_MAX_BRIGHTNESS = "max_brightness"
CONF_LUX_REFERENCE = "lux_reference"

HUB75_I2S_CFG = cg.global_ns.namespace("HUB75_I2S_CFG")
shift_driver = HUB75_I2S_CFG.enum("shift_driver")
line_driver = HUB75_I2S_CFG.enum("line_driver")
clk_speed = HUB75_I2S_CFG.enum("clk_speed")
PANEL_CHAIN_TYPE = cg.global_ns.enum("PANEL_CHAIN_TYPE")

SHIFT_DRIVERS = {
    "SHIFTREG": shift_driver.SHIFTREG,
    "FM6124": shift_driver.FM6124,
    "FM6126A": shift_driver.FM6126A,
    "ICN2038S": shift_driver.ICN2038S,
    "MBI5124": shift_driver.MBI5124,
    "DP3246": shift_driver.DP3246,
}

LINE_DRIVERS = {
    "TYPE138": line_driver.TYPE138,
    "TYPE595": line_driver.TYPE595,
    "TYPE_DIRECT": line_driver.TYPE_DIRECT,
    "SM5266P": line_driver.SM5266P,
    "SM5368": line_driver.SM5368,
}

CLOCK_SPEEDS = {
    "HZ_8M": clk_speed.HZ_8M,
    "HZ_10M": clk_speed.HZ_10M,
    "HZ_15M": clk_speed.HZ_15M,
    "HZ_16M": clk_speed.HZ_16M,
    "HZ_20M": clk_speed.HZ_20M,
}

CHAIN_TYPES = {
    "none": PANEL_CHAIN_TYPE.CHAIN_NONE,
    "top_left_down": PANEL_CHAIN_TYPE.CHAIN_TOP_LEFT_DOWN,
    "top_right_down": PANEL_CHAIN_TYPE.CHAIN_TOP_RIGHT_DOWN,
    "bottom_left_up": PANEL_CHAIN_TYPE.CHAIN_BOTTOM_LEFT_UP,
    "bottom_right_up": PANEL_CHAIN_TYPE.CHAIN_BOTTOM_RIGHT_UP,
    "top_left_down_zz": PANEL_CHAIN_TYPE.CHAIN_TOP_LEFT_DOWN_ZZ,
    "top_right_down_zz": PANEL_CHAIN_TYPE.CHAIN_TOP_RIGHT_DOWN_ZZ,
    "bottom_left_up_zz": PANEL_CHAIN_TYPE.CHAIN_BOTTOM_LEFT_UP_ZZ,
    "bottom_right_up_zz": PANEL_CHAIN_TYPE.CHAIN_BOTTOM_RIGHT_UP_ZZ,
}
validate_chain_type = cv.enum(CHAIN_TYPES, lower=True, space="_")

ROTATIONS = {0: 0, 90: 1, 180: 2, 270: 3}

SetBrightnessAction = hub75_dma_ns.class_("SetBrightnessAction", automation.Action)

PIN_SCHEMA = pins.internal_gpio_output_pin_schema


def _apply_board_defaults(config: ConfigType) -> ConfigType:
    board_name = config.get(CONF_BOARD)
    added: list[str] = []
    if board_name is not None:
        if board_name not in BOARDS:
            raise cv.Invalid(
                f"Unknown board '{board_name}'. Available: {', '.join(sorted(BOARDS))}"
            )
        for key, value in BOARDS[board_name].items():
            if key not in config:
                config[key] = value
                added.append(key)

    missing = [key for key in REQUIRED_PIN_KEYS if key not in config]
    if missing:
        raise cv.Invalid(
            "Missing HUB75 pins: "
            + ", ".join(missing)
            + ". Set board: or provide every pin (e_pin is optional except on 1/32-scan panels)."
        )

    for key in added:
        if key in PIN_KEYS:
            config[key] = PIN_SCHEMA(config[key])
        elif key == CONF_CLOCK_PHASE:
            config[key] = cv.boolean(config[key])
    return config


def _validate_panel_geometry(config: ConfigType) -> ConfigType:
    """One module is panel_width × panel_height. chain_cols × chain_rows is the grid.

    chain_length is a shortcut for one row (chain_cols). DMA always clocks a
    horizontal chain of rows×cols modules; chain_type remaps that into a stack.
    """
    rows = config.get(CONF_CHAIN_ROWS, 1)
    if CONF_CHAIN_COLS in config:
        cols = config[CONF_CHAIN_COLS]
        if CONF_CHAIN_LENGTH in config and config[CONF_CHAIN_LENGTH] != cols:
            raise cv.Invalid(
                "chain_length is an alias for chain_cols (modules in one row); "
                "set chain_rows for extra rows, not a different chain_length"
            )
    elif CONF_CHAIN_LENGTH in config:
        cols = config[CONF_CHAIN_LENGTH]
    else:
        cols = 1
    config[CONF_CHAIN_ROWS] = rows
    config[CONF_CHAIN_COLS] = cols
    config[CONF_CHAIN_LENGTH] = rows * cols
    chain_type = config.get(CONF_CHAIN_TYPE, "none")
    needs_virtual = (
        rows > 1
        or chain_type != "none"
        or config.get(CONF_CHAIN_ROTATION, 0) != 0
    )
    if needs_virtual and (
        config[CONF_PANEL_WIDTH] > 255 or config[CONF_PANEL_HEIGHT] > 255
    ):
        raise cv.Invalid(
            "stacked/rotated layouts remap through VirtualMatrixPanel, which "
            "needs each module ≤ 255 px on an edge"
        )
    if rows * cols > 16:
        raise cv.Invalid(
            f"chain_rows×chain_cols is {rows * cols}; at most 16 modules on the data path"
        )
    if CONF_CHAIN_TYPE not in config:
        config[CONF_CHAIN_TYPE] = validate_chain_type(
            "top_right_down" if rows > 1 else "none"
        )

    total_w = config[CONF_PANEL_WIDTH] * cols
    total_h = config[CONF_PANEL_HEIGHT] * rows
    rotation = config.get(CONF_CHAIN_ROTATION, 0)
    if rotation in (90, 270):
        total_w, total_h = total_h, total_w
    pixels = total_w * total_h
    board_name = config.get(CONF_BOARD)
    bounds = BOARD_BOUNDS.get(board_name) if board_name else None
    if bounds is not None:
        max_pixels = bounds["max_pixels"]
        max_edge = bounds["max_edge"]
        if total_w > max_edge or total_h > max_edge or pixels > max_pixels:
            raise cv.Invalid(
                f"board '{board_name}' supports at most {max_pixels} pixels "
                f"(neither edge above {max_edge}). Got {total_w}×{total_h} ({pixels} px) "
                f"from {cols}×{rows} modules of {config[CONF_PANEL_WIDTH]}×{config[CONF_PANEL_HEIGHT]}"
                f"{'' if rotation == 0 else f', rotation {rotation}'}."
            )
        if pixels > 128 * 64 and not config[CONF_USE_PSRAM]:
            raise cv.Invalid(
                f"{total_w}×{total_h} ({pixels} px) exceeds 128×64; set use_psram: true "
                "for the DMA framebuffer."
            )
    return config


CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Hub75DmaDisplay),
            cv.Required(CONF_PANEL_WIDTH): cv.int_range(min=8, max=384),
            cv.Required(CONF_PANEL_HEIGHT): cv.int_range(min=8, max=384),
            cv.Optional(CONF_BOARD): cv.string,
            cv.Optional(CONF_CHAIN_LENGTH): cv.int_range(min=1, max=16),
            cv.Optional(CONF_CHAIN_ROWS): cv.int_range(min=1, max=16),
            cv.Optional(CONF_CHAIN_COLS): cv.int_range(min=1, max=16),
            cv.Optional(CONF_CHAIN_TYPE): validate_chain_type,
            cv.Optional(CONF_CHAIN_ROTATION, default=0): cv.one_of(0, 90, 180, 270, int=True),
            cv.Optional(CONF_BRIGHTNESS, default=128): cv.int_range(min=0, max=255),
            cv.Optional(CONF_UPDATE_INTERVAL, default="16ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_DOUBLE_BUFFER, default=True): cv.boolean,
            cv.Optional(CONF_USE_PSRAM, default=False): cv.boolean,
            cv.Optional(CONF_R1_PIN): PIN_SCHEMA,
            cv.Optional(CONF_G1_PIN): PIN_SCHEMA,
            cv.Optional(CONF_B1_PIN): PIN_SCHEMA,
            cv.Optional(CONF_R2_PIN): PIN_SCHEMA,
            cv.Optional(CONF_G2_PIN): PIN_SCHEMA,
            cv.Optional(CONF_B2_PIN): PIN_SCHEMA,
            cv.Optional(CONF_A_PIN): PIN_SCHEMA,
            cv.Optional(CONF_B_PIN): PIN_SCHEMA,
            cv.Optional(CONF_C_PIN): PIN_SCHEMA,
            cv.Optional(CONF_D_PIN): PIN_SCHEMA,
            cv.Optional(CONF_E_PIN): PIN_SCHEMA,
            cv.Optional(CONF_LAT_PIN): PIN_SCHEMA,
            cv.Optional(CONF_OE_PIN): PIN_SCHEMA,
            cv.Optional(CONF_CLK_PIN): PIN_SCHEMA,
            cv.Optional(CONF_DRIVER): cv.enum(SHIFT_DRIVERS, upper=True, space="_"),
            cv.Optional(CONF_LINE_DECODER): cv.enum(LINE_DRIVERS, upper=True, space="_"),
            cv.Optional(CONF_I2SSPEED): cv.enum(CLOCK_SPEEDS, upper=True, space="_"),
            cv.Optional(CONF_LATCH_BLANKING): cv.int_range(min=1, max=4),
            cv.Optional(CONF_CLOCK_PHASE): cv.boolean,
            cv.Optional(CONF_ADAPTIVE_BRIGHTNESS): cv.Schema(
                {
                    cv.Required(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
                    cv.Optional(CONF_MIN_BRIGHTNESS, default=8): cv.int_range(min=0, max=255),
                    cv.Optional(CONF_MAX_BRIGHTNESS, default=255): cv.int_range(min=0, max=255),
                    cv.Optional(CONF_LUX_REFERENCE, default=500): cv.float_range(min=1.0),
                }
            ),
        }
    ),
    _apply_board_defaults,
    _validate_panel_geometry,
    cv.only_on_esp32,
)


async def to_code(config: ConfigType) -> None:
    add_dma_library()
    if config[CONF_USE_PSRAM]:
        cg.add_build_flag("-DSPIRAM_FRAMEBUFFER")

    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_panel_width(config[CONF_PANEL_WIDTH]))
    cg.add(var.set_panel_height(config[CONF_PANEL_HEIGHT]))
    cg.add(var.set_chain_length(config[CONF_CHAIN_LENGTH]))
    cg.add(var.set_chain_rows(config[CONF_CHAIN_ROWS]))
    cg.add(var.set_chain_cols(config[CONF_CHAIN_COLS]))
    cg.add(var.set_chain_type(config[CONF_CHAIN_TYPE]))
    cg.add(var.set_rotation(ROTATIONS[config[CONF_CHAIN_ROTATION]]))
    cg.add(var.set_initial_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_double_buffer(config[CONF_DOUBLE_BUFFER]))

    pin_args = []
    for key in PIN_KEYS:
        if key == CONF_E_PIN and key not in config:
            pin_args.append(0)
            continue
        pin_args.append(await cg.gpio_pin_expression(config[key]))
    cg.add(var.set_pins(*pin_args))

    if CONF_DRIVER in config:
        cg.add(var.set_shift_driver(config[CONF_DRIVER]))
    if CONF_LINE_DECODER in config:
        cg.add(var.set_line_decoder(config[CONF_LINE_DECODER]))
    if CONF_I2SSPEED in config:
        cg.add(var.set_i2sspeed(config[CONF_I2SSPEED]))
    if CONF_LATCH_BLANKING in config:
        cg.add(var.set_latch_blanking(config[CONF_LATCH_BLANKING]))
    if CONF_CLOCK_PHASE in config:
        cg.add(var.set_clock_phase(config[CONF_CLOCK_PHASE]))

    if CONF_ADAPTIVE_BRIGHTNESS in config:
        adaptive = config[CONF_ADAPTIVE_BRIGHTNESS]
        lux = await cg.get_variable(adaptive[CONF_SENSOR_ID])
        cg.add(var.set_adaptive_lux_sensor(lux))
        cg.add(
            var.set_adaptive_range(
                adaptive[CONF_MIN_BRIGHTNESS],
                adaptive[CONF_MAX_BRIGHTNESS],
                adaptive[CONF_LUX_REFERENCE],
            )
        )

    await display.register_display(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))


@automation.register_action(
    "hub75_dma.set_brightness",
    SetBrightnessAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Hub75DmaDisplay),
            cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.int_range(min=0, max=255)),
        },
        key=CONF_BRIGHTNESS,
    ),
    synchronous=True,
)
async def hub75_dma_set_brightness_to_code(
    config: ConfigType,
    action_id: ID,
    template_arg: cg.TemplateArguments,
    args: TemplateArgsType,
) -> MockObj:
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_BRIGHTNESS], args, cg.uint8)
    cg.add(var.set_brightness(template_))
    return var
