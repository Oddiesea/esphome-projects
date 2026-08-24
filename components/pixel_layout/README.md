# pixel_layout

Widget-tree compositor for any ESPHome `display`. Draw a Pixlet-style stack/row/column of clocks, dates, weather icons, text (static or live sensors), icons, and sprites into an offscreen RGB buffer, then blit. Designed for coarse LED matrices (first consumer: 128×64 HUB75 via [`hub75_dma`](../hub75_dma/)).

The compositor **owns the frame cadence**: it stops the display poller and only paints when a widget is dirty (sensor/text callback, clock colon/minute, date rollover, sprite frame, running animation). Sensor values are formatted on `add_on_state_callback`, not by polling HA or `get_state()` in `draw()`. Blit uses `draw_pixels_at` (HUB75 skips the per-pixel WDT feed).

**Custom** pixel widgets store a packed `p4:` bitmap in `pixels:` (4-bit palette indices, base64). Width and height are in the bitmap header. A list of ASCII rows (`.` / `X` / `1`–`9`) still loads. Palette color 1 is `color:`; extras 2–9 are `palette:`.

**Sprites** can be an ESPHome `image:` / `animation:` id, or a **sprite pack** inlined as YAML (`pack:`). The pack holds a base64 PNG plus sheet size, cut grid (`columns` / `rows` / `cuts`), `frames`, `fps`, `loop`, and `chroma_key`. Firmware extracts the PNG at compile time — no separate `image:` block. `pack:` may be the mapping itself, `!include assets/spinner.yml`, or a path to a `.yml` / `.json` file. PNG sheets live in the device (or configurator) YAML, not in this component. The configurator Copy YAML inlines the same mapping.

Digital clocks use **themes** (typefaces), not separate clock types: `seven_segment`, `rounded`, `block`, `tiny`, `typeface`, `split_flap`, `perspective`. `seven_segment`, `rounded`, `block`, `split_flap`, and `perspective` take nested `size: sm | md | lg` (default `md`). `split_flap` draws each digit as a card and flips a module when that digit changes. `perspective` is a tall tapered 7-segment face (wide top, narrow bottom) with thicker top bars (7/4/2 at `md`, taper 10 → ~4px base). Analog is `face: analog` with `theme: ring` (bezel + hour dots), `minimal` (12/3/6/9 ticks), `ticks` (all 12 hour marks), or `square` (square bezel). Set `outline: black` or `outline: white` for a 1px halo on digits, typefaces, hands, side icons, and other text/icon widgets (`true` still means black). Set `ghost: true` on `seven_segment`, `rounded`, or `perspective` to keep unused bars dimly lit. Metrics live in [`themes.json`](themes.json).

**Icons** are Material Symbols ligature names (`home`, `thermostat`, `lightbulb`). Weather aliases (`thermometer`, `partlycloudy`) still work. The configurator search lists the full symbol set; Copy YAML emits only the glyphs you used.

**Text** is the one content widget for copy and live values. Bind a numeric `sensor_id` and/or a `text_sensor_id`, then write a template in `text:`:

- `{value}` / `{value:.1f}` / `{value:.0f}` — numeric state (`--` if missing)
- `{state}` / `{text}` — text sensor (or string state)
- `{unit}` `{label}` — optional `unit:` and `label:` on the widget
- `{{` / `}}` — literal braces; a newline (or `\n`) draws two lines

Any widget can optionally take `visible:` so it only draws when Home Assistant (or local) sensors match. A single condition is enough:

```yaml
visible:
  sensor_id: outdoor_temp
  equal: 21.5
```

```yaml
visible:
  sensor_id: outdoor_temp
  at_least: 15
  at_most: 28
```

```yaml
visible:
  text_sensor_id: occupancy
  state: "on"
```

Combine with `and:` (all must match) or `or:` (any may match). Each item can set `invert: true`. The whole group can also `invert: true`.

```yaml
visible:
  and:
    - sensor_id: outdoor_temp
      above: 15
    - text_sensor_id: occupancy
      state: "on"
```

```yaml
visible:
  or:
    - text_sensor_id: alarm
      state: "disarmed"
    - text_sensor_id: occupancy
      state: "on"
      invert: true
```

`above` (`>`), `below` (`<`), `equal` (`=`), `not_equal` (`≠`), `at_least` (`≥`), and `at_most` (`≤`) compare a numeric `sensor_id`. `equals:` is an alias for `equal:`. Unknown or missing state fails that condition. A text sensor uses `state:` instead. The configurator **Show when** picker writes this block and the `sensor:` / `text_sensor:` `homeassistant` imports.

Legacy `format: "%.1f°"` still works when `text` is empty. `type: sensor` is an alias for `type: text`. **Date** is its own widget (`style: text | two_line | calendar`, `format`, `uppercase`, `show_year`). **Shape** (`type: shape` or `box`) draws `kind: rect | rounded | oval | pill | triangle | diamond | plus | frame | ring | line`. `radius` rounds `rounded` corners. `point: up | down | left | right` aims a triangle. `stroke` is the line weight on `plus`, `frame`, `ring`, and `line` (1–32). `antialias` softens oval, rounded, pill, triangle, diamond, and ring edges. Optional `padding` insets a nested `child:` (firmware layout only). **Weather** maps a Home Assistant weather `text_sensor` (or static `condition:`) to a Material icon (`sunny`, `partlycloudy`, `rainy`, …).

Widget `animation:` is `fade`, `slide`, `pulse`, or `blink`. Fade and slide take `mode: in | out | in_out` (`inout` / `both` aliases). `in` goes from→to (fade) or offset→rest (slide); `out` is the reverse; `in_out` does both in one `duration`. Pulse and blink ignore `mode`.

```yaml
animation:
  type: fade
  mode: in_out
  duration: 400ms
```

**Screens** rotate on the device: `screens:` is a list of widget trees, `rotate:` is the default dwell, and `transition: fade | cut | slide_left | slide_right | slide_up | slide_down | wipe_left | wipe_right | wipe_up | wipe_down | iris | dissolve | blinds` (`slide` / `wipe` alias left). Each screen may set its own `transition` / `transition_duration`; that effect is used when **leaving** that screen. Omitted values use the layout default. `loop: false` stops on the last screen instead of wrapping. `random: true` picks the next screen at random (with `loop: false` it plays each screen once). Premade layouts live as ordinary widget YAML in the configurator `assets/screens/` folder. `root:` still means a single screen.

Preview layouts in [`configurators/pixel_layout`](../../configurators/pixel_layout/). The configurator `make serve` proxies Home Assistant so sensor, weather, and text widgets can pick live entities (URL and token from the UI, stored in the browser). Sprite widgets export YAML packs (PNG + cuts + fps); Copy YAML inlines `pack:` into the device config. Premade screens are YAML files in the configurator `assets/screens/` folder.

**Text** is drawn with the selected typeface (not the 3×5 digit font). **WLED 16** (May 2026) is useful as a matrix-driver reference, not as a compositor: dedicated HUB75 DMA binaries, ping-pong/double buffers, high-priority output so Wi‑Fi does not starve the shift clock, PSRAM for large frames, and “paint only when dirty.” `hub75_dma` already double-buffers and flips DMA; `pixel_layout` already owns cadence. Do not copy WLED’s per-LED particle effects onto HUB75 — they are sized for addressable strips.
