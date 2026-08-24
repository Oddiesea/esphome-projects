"""Optional pin presets for common ESP32-S3 HUB75 driver boards.

Pins can always be set manually on the display platform. A board name only
fills in defaults for pins (and clock_phase) that the YAML does not override.
"""

from esphome.const import CONF_CLK_PIN, CONF_OE_PIN

CONF_R1_PIN = "r1_pin"
CONF_G1_PIN = "g1_pin"
CONF_B1_PIN = "b1_pin"
CONF_R2_PIN = "r2_pin"
CONF_G2_PIN = "g2_pin"
CONF_B2_PIN = "b2_pin"
CONF_A_PIN = "a_pin"
CONF_B_PIN = "b_pin"
CONF_C_PIN = "c_pin"
CONF_D_PIN = "d_pin"
CONF_E_PIN = "e_pin"
CONF_LAT_PIN = "lat_pin"
CONF_CLOCK_PHASE = "clock_phase"

PIN_KEYS = [
    CONF_R1_PIN,
    CONF_G1_PIN,
    CONF_B1_PIN,
    CONF_R2_PIN,
    CONF_G2_PIN,
    CONF_B2_PIN,
    CONF_A_PIN,
    CONF_B_PIN,
    CONF_C_PIN,
    CONF_D_PIN,
    CONF_E_PIN,
    CONF_LAT_PIN,
    CONF_OE_PIN,
    CONF_CLK_PIN,
]

REQUIRED_PIN_KEYS = [key for key in PIN_KEYS if key != CONF_E_PIN]

# Waveshare ESP32-S3-RGB-Matrix (SKU 34422). GPIO 3 is a strapping pin on
# ESP32-S3 but is wired to HUB75 C on this board.
#
# Numbered grid with adequate 5V: silk G→red, silk B→green, silk R→blue.
WAVESHARE_ESP32_S3_RGB_MATRIX = {
    CONF_R1_PIN: 5,  # silkscreen G1 — red LEDs
    CONF_G1_PIN: 6,  # silkscreen B1 — green LEDs
    CONF_B1_PIN: 4,  # silkscreen R1 — blue LEDs
    CONF_R2_PIN: 15,  # silkscreen G2
    CONF_G2_PIN: 16,  # silkscreen B2
    CONF_B2_PIN: 7,  # silkscreen R2
    CONF_A_PIN: 18,
    CONF_B_PIN: 8,
    CONF_C_PIN: {"number": 3, "ignore_strapping_warning": True},
    CONF_D_PIN: 42,
    CONF_E_PIN: 9,
    CONF_LAT_PIN: 40,
    CONF_OE_PIN: 2,
    CONF_CLK_PIN: 41,
    CONF_CLOCK_PHASE: False,
}

BOARDS = {
    "waveshare-esp32-s3-rgb-matrix": WAVESHARE_ESP32_S3_RGB_MATRIX,
}

# Total pixel budget for this driver, not a landscape box or a 1D chain.
# (panel_width × chain_cols) × (panel_height × chain_rows) ≤ max_pixels
# after rotation, and neither logical edge exceeds max_edge.
BOARD_BOUNDS = {
    "waveshare-esp32-s3-rgb-matrix": {
        "max_pixels": 384 * 64,
        "max_edge": 384,
    },
}
