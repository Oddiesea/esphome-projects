import { jsyaml } from "./yaml_pkg.js";
import { CLOCK_SIZE_THEMES, DEFAULT_FONT, DEFAULT_ICON_FONT, ICON_FONTS, ICON_GLYPHS, analogTheme, defaultsFor, decodePixelValue, iconCodepoint, iconFontSpec, iconLigature, normalizeColon, normalizeOutline, normalizeTransition, packPixelBitmap, pixelColors, pixelIndex, typefaceSpec, weatherIconName, widgetSupportsAnim } from "./schema.js";
import { spriteFileHint, spritePackForWidget } from "./sprites.js";
import { visibleToYaml, normalizeVisible, ruleToYaml } from "./ha.js";
import { exampleById, substitutionKey } from "./example_sensors.js";
import { ROOT_DEFAULTS } from "./root_defaults.js";

export { ROOT_DEFAULTS };

export const HUB75_BOARD_OPTIONS = [
  { value: "waveshare-esp32-s3-rgb-matrix", label: "Waveshare ESP32-S3 RGB Matrix" },
];

const WEATHER_FALLBACK_GLYPHS = Object.keys(ICON_GLYPHS).filter(
  (k) => !["schedule", "thermometer", "water_drop", "home", "wifi"].includes(k),
);

function num(v) {
  const n = Number(v);
  return Number.isFinite(n) ? n : 0;
}

export function allWidgets(state) {
  if (state.screens?.length) return state.screens.flatMap((s) => s.widgets || []);
  return state.widgets || [];
}

/** Root SNTP / time component id shared by clock and date widgets. */
export function effectiveTimeId(state) {
  return state?.time_id || ROOT_DEFAULTS.time_id;
}

/** Pick up time_id from imported widgets when the root field is unset. */
export function syncRootTimeId(state) {
  if (state.time_id) return;
  for (const w of allWidgets(state)) {
    if ((w.type === "clock" || w.type === "date") && w.time_id) {
      state.time_id = w.time_id;
      return;
    }
  }
  state.time_id = ROOT_DEFAULTS.time_id;
}

function durationYaml(ms) {
  const n = Number(ms) || 8000;
  if (n % 1000 === 0) return `${n / 1000}s`;
  return `${n}ms`;
}

function applySideIconsYaml(o, w) {
  if (w.icon) o.icon = w.icon;
  if (w.icon_end) o.icon_end = w.icon_end;
  if (!w.icon && !w.icon_end) return;
  if (w.icon_color) o.icon_color = w.icon_color;
  if (w.icon_font) o.icon_font = w.icon_font;
  if (w.icon_align && w.icon_align !== "middle") o.icon_align = w.icon_align;
  if (w.text_align && w.text_align !== "middle") o.text_align = w.text_align;
  if (w.icon_gap != null && Number(w.icon_gap) !== 2) o.icon_gap = Number(w.icon_gap);
}

function animToYaml(anim) {
  if (!anim || !anim.type) return null;
  const o = { type: anim.type };
  if (anim.duration && anim.duration !== "400ms") o.duration = anim.duration;
  if (anim.delay && anim.delay !== "0ms") o.delay = anim.delay;
  if (anim.repeat != null && Number(anim.repeat) !== 0) o.repeat = num(anim.repeat);
  if (anim.from != null && Number(anim.from) !== 0) o.from = num(anim.from);
  if (anim.to != null && Number(anim.to) !== 255) o.to = num(anim.to);
  if (anim.direction) o.direction = anim.direction;
  if (anim.mode && anim.mode !== "in") o.mode = anim.mode === "inout" || anim.mode === "both" ? "in_out" : anim.mode;
  if (num(anim.dx)) o.dx = num(anim.dx);
  if (num(anim.dy)) o.dy = num(anim.dy);
  return o;
}

export function widgetToYaml(w, layoutState) {
  const d = defaultsFor(w.type);
  const timeId = effectiveTimeId(layoutState);
  const o = { type: w.type };
  o.x = num(w.x);
  o.y = num(w.y);
  if (w.width) o.width = num(w.width);
  if (w.height) o.height = num(w.height);
  if (w.opacity != null && num(w.opacity) !== 255) o.opacity = num(w.opacity);
  if (w.expanded) o.expanded = true;
  if (w.color) o.color = w.color;
  if (["clock", "text", "icon", "date", "weather"].includes(w.type)) {
    const outline = normalizeOutline(w.outline);
    if (outline === "black" || outline === "white") o.outline = outline;
  }

  if (w.type === "clock") {
    o.time_id = w.time_id || timeId;
    if (w.fallback_time_id) o.fallback_time_id = w.fallback_time_id;
    o.face = w.face || "digital";
    if (w.secondary_color) o.secondary_color = w.secondary_color;
    if (w.ghost && o.face === "digital" && (w.theme === "seven_segment" || w.theme === "rounded" || w.theme === "perspective" || !w.theme)) o.ghost = true;
    if (w.show_seconds) o.show_seconds = true;
    if (o.face === "digital") {
      o.theme = w.theme || "seven_segment";
      const colon = normalizeColon(w);
      if (colon === "solid") o.colon = "solid";
      else if (colon === "off") o.colon = "off";
      else o.blink_colon = true;
      if (CLOCK_SIZE_THEMES.has(o.theme) && w.size && w.size !== "md") {
        o.size = w.size;
      }
      if (o.theme === "typeface") {
        o.font = w.font || d.font;
        o.format = w.format || (w.show_seconds ? "%H:%M:%S" : "%H:%M");
      } else if (w.format) {
        o.format = w.format;
        o.font = w.font || d.font;
      }
    } else {
      o.theme = analogTheme(w.theme);
    }
    applySideIconsYaml(o, w);
  }

  if (w.type === "text") {
    o.type = "text";
    if (w.text) o.text = w.text;
    if (w.source === "number" && (w.sensor_id || d.sensor_id)) o.sensor_id = w.sensor_id || d.sensor_id;
    if (w.source === "string" && w.text_sensor_id) o.text_sensor_id = w.text_sensor_id;
    if (w.label) o.label = w.label;
    if (w.style && w.style !== "text") o.style = w.style;
    if (w.unit) o.unit = w.unit;
    if (w.font) o.font = w.font;
    applySideIconsYaml(o, w);
  }

  if (w.type === "icon") {
    o.icon = iconLigature(w.icon) || w.icon || "schedule";
    if (w.font) o.font = w.font;
    if (w.icon_font) o.icon_font = w.icon_font;
  }

  if (w.type === "custom") {
    const rows = Array.isArray(w.pixels) ? w.pixels : decodePixelValue(w.pixels, w.width, w.height);
    if (rows.some((row) => [...String(row)].some((c) => pixelIndex(c) > 0))) {
      o.pixels = packPixelBitmap(rows, w.width, w.height);
    }
    const extras = pixelColors(w).slice(1);
    if (extras.length) o.palette = extras;
  }

  if (w.type === "sprite") {
    const pack = spritePackForWidget(w);
    if (pack) o.pack = pack;
    else if (w.animation_id) o.animation_id = w.animation_id;
    else o.image_id = w.image_id || "spinner_sheet";
    if (!pack) {
      if (w.frame_width) o.frame_width = num(w.frame_width);
      if (w.frame_height) o.frame_height = num(w.frame_height);
      if (w.frames) o.frames = num(w.frames);
      if (num(w.frames) > 1 && w.fps != null && w.fps !== "" && num(w.fps) > 0) o.fps = num(w.fps);
      if (w.loop === false) o.loop = false;
    }
  }

  if (w.type === "box" || w.type === "shape") {
    o.type = "shape";
    o.kind = w.kind || "rect";
    if (w.fill) o.fill = w.fill;
    if (w.padding) o.padding = num(w.padding);
    if (o.kind === "rounded" && w.radius) o.radius = num(w.radius);
    if (o.kind === "triangle" && w.point && w.point !== "up") o.point = w.point;
    if (["plus", "frame", "ring", "line"].includes(o.kind) && w.stroke) o.stroke = num(w.stroke);
    if (w.antialias) o.antialias = true;
  }

  if (w.type === "date") {
    o.time_id = w.time_id || timeId;
    o.style = w.style || "text";
    if (o.style !== "calendar") o.format = w.format || "%a %d %b";
    if (w.uppercase) o.uppercase = true;
    if (w.font) o.font = w.font;
    applySideIconsYaml(o, w);
  }

  if (w.type === "weather") {
    if (w.condition_id) o.condition_id = w.condition_id;
    if (w.condition) o.condition = w.condition;
    if (!o.condition_id && !o.condition) o.condition = "sunny";
    if (w.show_icon === false) o.show_icon = false;
    if (w.show_condition !== false) o.show_condition = true;
    if (w.show_temp) {
      o.show_temp = true;
      if (w.temperature_id) o.temperature_id = w.temperature_id;
    }
    if (w.show_humidity) {
      o.show_humidity = true;
      if (w.humidity_id) o.humidity_id = w.humidity_id;
    }
    if (w.show_wind) {
      o.show_wind = true;
      if (w.wind_speed_id) o.wind_speed_id = w.wind_speed_id;
      if (w.wind_bearing_id) o.wind_bearing_id = w.wind_bearing_id;
    }
    const textPos = w.text_position || "end";
    if (textPos !== "end") o.text_position = textPos;
    if (w.icon_align && w.icon_align !== "middle") o.icon_align = w.icon_align;
    if (w.text_align && w.text_align !== "middle") o.text_align = w.text_align;
    const gap = Number(w.gap);
    if (Number.isFinite(gap) && gap !== 2) o.gap = gap | 0;
    if (w.icon_theme) o.icon_theme = w.icon_theme;
    else if (w.icon_font) o.icon_font = w.icon_font;
    if (w.icons && typeof w.icons === "object") {
      const icons = {};
      for (const [rawKey, spec] of Object.entries(w.icons)) {
        if (!spec || typeof spec !== "object") continue;
        const rows = Array.isArray(spec.pixels)
          ? spec.pixels
          : decodePixelValue(spec.pixels, spec.width, spec.height);
        if (!rows.some((row) => [...String(row)].some((c) => pixelIndex(c) > 0))) continue;
        const width = Number(spec.width) || rows[0]?.length || 16;
        const height = Number(spec.height) || rows.length || 16;
        const entry = {
          width,
          height,
          pixels: packPixelBitmap(rows, width, height),
        };
        if (spec.color) entry.color = spec.color;
        if (Array.isArray(spec.palette) && spec.palette.length) entry.palette = spec.palette;
        icons[rawKey] = entry;
      }
      if (Object.keys(icons).length) o.icons = icons;
    }
    const anyText =
      w.show_condition !== false || w.show_temp || w.show_humidity || w.show_wind;
    if (anyText) o.font = w.font || DEFAULT_FONT;
  }

  const anim = widgetSupportsAnim(w.type) ? animToYaml(w.animation) : null;
  if (anim) o.animation = anim;
  const vis = visibleToYaml(w.visible);
  if (vis) o.visible = vis;
  return o;
}

/** Premade screen YAML (`name`, `duration`, `widgets`) for import/export. */
export function screenToYamlDoc(screen, layoutState) {
  const doc = {
    name: screen?.name || "Screen",
    duration: durationYaml(screen?.duration_ms || layoutState?.rotate_ms || 8000),
    widgets: (screen?.widgets || []).map((w) => widgetToYaml(w, layoutState)),
  };
  if (screen?.blurb) doc.blurb = screen.blurb;
  const kind = screen?.transition ? normalizeTransition(screen.transition) : null;
  if (kind) doc.transition = kind;
  if (screen?.transition_ms != null) {
    doc.transition_duration = durationYaml(screen.transition_ms);
  }
  return doc;
}

function glyphLine(name) {
  const liga = iconLigature(name) || name;
  const cp = iconCodepoint(name);
  if (!cp) return `      - "${liga}"  # ${liga}`;
  return `      - "\\U${cp.toString(16).padStart(8, "0")}"  # ${liga}`;
}

function collectIconUsage(state) {
  const byFont = new Map();
  const add = (fontId, names) => {
    const spec = iconFontSpec(fontId || state.icon_font || DEFAULT_ICON_FONT);
    if (!byFont.has(spec.id)) byFont.set(spec.id, { spec, names: new Set() });
    for (const n of names) {
      if (n) byFont.get(spec.id).names.add(n);
    }
  };
  add(state.icon_font, []);
  for (const w of allWidgets(state)) {
    if (w.type === "icon") add(w.icon_font, [w.icon]);
    if (w.type === "text" && (w.icon || w.icon_end)) add(w.icon_font, [w.icon, w.icon_end]);
    if (w.type === "weather") {
      // Always ship Material weather glyphs as fallback even when icon_theme is set.
      if (w.condition_id || w.icon_theme) add(w.icon_font, WEATHER_FALLBACK_GLYPHS);
      else add(w.icon_font, [weatherIconName(w.condition)]);
    }
    if ((w.type === "clock" || w.type === "date") && (w.icon || w.icon_end)) add(w.icon_font, [w.icon, w.icon_end]);
  }
  return byFont;
}

export function fontImportsYaml(state) {
  const ids = new Set();
  if (state.font) ids.add(state.font);
  if (state.icon_font) ids.add(state.icon_font);
  for (const w of allWidgets(state)) {
    if (w.font) ids.add(w.font);
    if (w.icon_font) ids.add(w.icon_font);
  }
  const faces = [];
  const seenFace = new Set();
  const iconFamilyIds = new Set(ICON_FONTS.map((f) => f.id));
  for (const id of ids) {
    const raw = String(id || "");
    const familyGuess = raw.replace(/_\d+$/, "");
    if (iconFamilyIds.has(familyGuess) || iconFamilyIds.has(raw)) continue;
    const spec = typefaceSpec(id);
    if (!seenFace.has(spec.id)) {
      seenFace.add(spec.id);
      faces.push(spec);
    }
  }
  const icons = [...collectIconUsage(state).values()];
  // Ensure layout icon_font appears even when no icon glyphs were collected.
  if (state.icon_font && !icons.some((row) => row.spec.id === iconFontSpec(state.icon_font).id)) {
    icons.push({ spec: iconFontSpec(state.icon_font), names: new Set(["schedule"]) });
  }
  if (!faces.length && !icons.length) return "";
  const lines = ["font:"];
  for (const spec of faces) {
    lines.push(`  - file: "gfonts://${spec.gfonts}"`);
    lines.push(`    id: ${spec.id}`);
    lines.push(`    size: ${spec.size}`);
    if (spec.weight >= 700) lines.push(`    weight: bold`);
    if (spec.bpp === 1) lines.push(`    bpp: 1`);
  }
  for (const { spec, names } of icons) {
    const glyphs = [...names].sort();
    if (!glyphs.length) glyphs.push("schedule");
    lines.push(`  - file: "gfonts://${spec.gfonts}"`);
    lines.push(`    id: ${spec.id}`);
    lines.push(`    size: ${spec.size}`);
    lines.push(`    glyphsets: []`);
    lines.push(`    glyphs:`);
    for (const name of glyphs) lines.push(glyphLine(name));
  }
  lines.push("");
  return lines.join("\n");
}

/** ESPHome time: blocks for every clock/date time_id (+ fallback). */
export function timeImportsYaml(state) {
  const ids = new Set();
  for (const w of allWidgets(state)) {
    if (w.type !== "clock" && w.type !== "date") continue;
    if (w.time_id) ids.add(String(w.time_id).trim());
    if (w.fallback_time_id) ids.add(String(w.fallback_time_id).trim());
  }
  if (!ids.size) {
    const root = effectiveTimeId(state);
    if (root) ids.add(root);
  }
  ids.delete("");
  if (!ids.size) return "";
  const haIds = [];
  const sntpIds = [];
  for (const id of [...ids].sort()) {
    if (/^ha[_-]?/i.test(id) || /home.?assistant/i.test(id)) haIds.push(id);
    else sntpIds.push(id);
  }
  const lines = ["time:"];
  for (const id of haIds) {
    lines.push(`  - platform: homeassistant`);
    lines.push(`    id: ${id}`);
  }
  for (const id of sntpIds) {
    lines.push(`  - platform: sntp`);
    lines.push(`    id: ${id}`);
    lines.push(`    servers:`);
    lines.push(`      - time.cloudflare.com`);
    lines.push(`      - 0.pool.ntp.org`);
  }
  lines.push("");
  return lines.join("\n");
}

function layoutYamlOnly(state) {
  const root = {
    id: "matrix_layout",
    display_id: state.display_id || ROOT_DEFAULTS.display_id,
    background: state.background || ROOT_DEFAULTS.background,
  };
  if (state.font) root.font = state.font;
  if (state.icon_font) root.icon_font = state.icon_font;
  const screens = state.screens?.length
    ? state.screens
    : [{ name: "Screen 1", duration_ms: state.rotate_ms || 8000, widgets: state.widgets || [] }];
  root.rotate = durationYaml(state.rotate_ms || 8000);
  const trans = normalizeTransition(state.transition);
  root.transition = trans;
  if (state.transition_ms != null && Number(state.transition_ms) !== 400) {
    root.transition_duration = durationYaml(state.transition_ms);
  }
  if (state.loop === false) root.loop = false;
  if (state.random) root.random = true;
  root.screens = (() => {
    const used = new Set();
    return screens.map((s, i) => {
      let id = slugScreenId(s.name) || `screen_${i + 1}`;
      const base = id;
      let n = 2;
      while (used.has(id)) {
        id = `${base}_${n}`;
        n += 1;
      }
      used.add(id);
      const row = {
        id,
        duration: durationYaml(s.duration_ms || state.rotate_ms || 8000),
        root: {
          type: "stack",
          children: (s.widgets || []).map((w) => widgetToYaml(w, state)),
        },
      };
      const kind = s.transition ? normalizeTransition(s.transition) : null;
      if (kind) row.transition = kind;
      if (s.transition_ms != null) {
        row.transition_duration = durationYaml(s.transition_ms);
      }
      return row;
    });
  })();
  return jsyaml.dump({ pixel_layout: root }, { lineWidth: 88, noRefs: true, quotingType: '"' });
}

/** Stable unique screen ids matching layoutYamlOnly export. */
export function exportedScreenIds(state) {
  const screens = state.screens?.length
    ? state.screens
    : [{ name: "Screen 1", duration_ms: state.rotate_ms || 8000, widgets: state.widgets || [] }];
  const used = new Set();
  return screens.map((s, i) => {
    let id = slugScreenId(s.name) || `screen_${i + 1}`;
    const base = id;
    let n = 2;
    while (used.has(id)) {
      id = `${base}_${n}`;
      n += 1;
    }
    used.add(id);
    return id;
  });
}

function slugScreenId(name) {
  return String(name || "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "_")
    .replace(/^_|_$/g, "");
}

function clampBri(n, fallback = 80) {
  const v = Number(n);
  return Math.max(0, Math.min(255, Number.isFinite(v) ? v : fallback));
}

function clampComp(n) {
  const v = Number(n);
  if (!Number.isFinite(v)) return ROOT_DEFAULTS.brightness_compensation;
  return Math.max(0.1, Math.min(5, Math.round(v * 100) / 100));
}

export function adaptiveBrightnessEnabled(state) {
  return Boolean(state?.adaptive_brightness);
}

/** Full paste-ready device package: substitutions + time + fonts + HA + hub75 + pixel_layout. */
export function toYaml(state, opts = {}) {
  const panelWidth = Math.max(1, Number(opts.panelWidth ?? state.panel_width) || 128);
  const panelHeight = Math.max(1, Number(opts.panelHeight ?? state.panel_height) || 64);
  const brightness = clampBri(state.brightness ?? ROOT_DEFAULTS.brightness);
  const ha = collectHaImports(state);
  const adaptive = adaptiveBrightnessEnabled(state);
  const compensation = clampComp(state.brightness_compensation ?? ROOT_DEFAULTS.brightness_compensation);

  const substs = new Map([
    ["panel_width", String(panelWidth)],
    ["panel_height", String(panelHeight)],
    ["brightness", String(brightness)],
  ]);
  if (adaptive) substs.set("brightness_compensation", String(compensation));
  for (const [key, value] of ha.substitutions) {
    if (!substs.has(key)) substs.set(key, value);
  }

  const parts = [
    "# pixel_layout device package — paste into the ESPHome editor or !include as a package.",
    "# One substitutions block (panel size, brightness, HA entities). Remap entity_ids to yours.",
    "# Sprite packs are inlined when loaded in the configurator.",
    adaptive
      ? "# Adaptive brightness: add your own i2c + lux sensor (e.g. veml7700) with id matching sensor_id below."
      : "",
    "",
    substitutionsYaml(substs),
    timeImportsYaml(state),
    fontImportsYaml(state),
    imageImportsYaml(state),
    formatHaSensorsYaml(ha.items),
    hub75DeviceYaml(state),
    layoutYamlOnly(state),
  ].filter(Boolean);
  return parts.join("\n").replace(/\n{3,}/g, "\n\n");
}

function substitutionsYaml(substs) {
  if (!substs?.size) return "";
  const lines = ["substitutions:"];
  for (const [key, value] of substs) {
    const needsQuote = /[:#{}[\],&*?|<>=!%@`]/.test(String(value)) || /^\d/.test(String(value));
    lines.push(needsQuote ? `  ${key}: "${value}"` : `  ${key}: ${value}`);
  }
  lines.push("");
  return lines.join("\n");
}

/** hub75_dma display + brightness/power (+ optional adaptive) + pixel_layout playlist HA controls. */
export function hub75DeviceYaml(state) {
  const id = state.display_id || ROOT_DEFAULTS.display_id;
  const board = state.hub75_board || ROOT_DEFAULTS.hub75_board;
  const adaptive = adaptiveBrightnessEnabled(state);
  const luxId =
    String(state.adaptive_lux_sensor_id || ROOT_DEFAULTS.adaptive_lux_sensor_id || "ambient_lux").trim() ||
    "ambient_lux";
  const minB = clampBri(state.adaptive_brightness_min ?? ROOT_DEFAULTS.adaptive_brightness_min, 8);
  const maxB = clampBri(state.adaptive_brightness_max ?? ROOT_DEFAULTS.adaptive_brightness_max, 255);
  const lo = Math.min(minB, maxB);
  const hi = Math.max(minB, maxB);
  const screenIds = exportedScreenIds(state);
  const plId = "matrix_layout";
  const lines = [
    "display:",
    "  - platform: hub75_dma",
    `    id: ${id}`,
    `    board: ${board}`,
    "    panel_width: ${panel_width}",
    "    panel_height: ${panel_height}",
    "    brightness: ${brightness}",
    "    update_interval: 1s  # paint poller; pixel_layout stops it. DMA scan stays ~60 Hz.",
  ];
  if (adaptive) {
    lines.push(
      "    adaptive_brightness:",
      `      sensor_id: ${luxId}`,
      `      min_brightness: ${lo}`,
      `      max_brightness: ${hi}`,
      "      lux_reference: 500",
    );
  }
  lines.push(
    "",
    "number:",
    "  - platform: hub75_dma",
    `    hub75_dma_id: ${id}`,
    '    name: "Brightness"',
  );
  if (adaptive) {
    lines.push(
      "  - platform: hub75_dma",
      "    type: compensation",
      `    hub75_dma_id: ${id}`,
      '    name: "Brightness compensation"',
      "    initial_value: ${brightness_compensation}",
    );
  }
  lines.push(
    "  - platform: pixel_layout",
    "    type: rotate_interval",
    `    pixel_layout_id: ${plId}`,
    '    name: "Rotate interval"',
    "  - platform: pixel_layout",
    "    type: transition_duration",
    `    pixel_layout_id: ${plId}`,
    '    name: "Transition duration"',
  );
  lines.push(
    "",
    "switch:",
    "  - platform: hub75_dma",
    `    hub75_dma_id: ${id}`,
    '    name: "Power"',
    "    restore_mode: ALWAYS_ON",
  );
  if (adaptive) {
    lines.push(
      "  - platform: hub75_dma",
      "    type: adaptive",
      `    hub75_dma_id: ${id}`,
      '    name: "Adaptive brightness"',
    );
  }
  lines.push(
    "  - platform: pixel_layout",
    "    type: pin",
    `    pixel_layout_id: ${plId}`,
    '    name: "Pin screen"',
    "  - platform: pixel_layout",
    "    type: random",
    `    pixel_layout_id: ${plId}`,
    '    name: "Random order"',
    "  - platform: pixel_layout",
    "    type: override_rotate",
    `    pixel_layout_id: ${plId}`,
    '    name: "Override rotate interval"',
    "  - platform: pixel_layout",
    "    type: override_transition",
    `    pixel_layout_id: ${plId}`,
    '    name: "Override transition"',
  );
  screenIds.forEach((sid, i) => {
    lines.push(
      "  - platform: pixel_layout",
      "    type: screen_enabled",
      `    pixel_layout_id: ${plId}`,
      `    screen_index: ${i}`,
      `    name: "Screen: ${sid}"`,
    );
  });
  lines.push(
    "",
    "select:",
    "  - platform: pixel_layout",
    "    type: screen",
    `    pixel_layout_id: ${plId}`,
    '    name: "Screen"',
    "    options:",
  );
  for (const sid of screenIds) {
    lines.push(`      - ${sid}`);
  }
  lines.push(
    "  - platform: pixel_layout",
    "    type: transition",
    `    pixel_layout_id: ${plId}`,
    '    name: "Transition"',
  );
  lines.push(
    "",
    "button:",
    "  - platform: restart",
    '    name: "Reboot"',
    "  - platform: pixel_layout",
    "    type: next",
    `    pixel_layout_id: ${plId}`,
    '    name: "Next screen"',
    "",
  );
  return lines.join("\n");
}

export function imageImportsYaml(state) {
  const images = [];
  const animations = [];
  const seenImg = new Set();
  const seenAnim = new Set();
  for (const w of allWidgets(state)) {
    if (w.type !== "sprite") continue;
    if (w.animation_id) {
      if (seenAnim.has(w.animation_id)) continue;
      seenAnim.add(w.animation_id);
      animations.push({
        id: w.animation_id,
        file: spriteFileHint(w.image_id || w.animation_id),
      });
      continue;
    }
    if (spritePackForWidget(w)) continue;
    const id = w.image_id || "spinner_sheet";
    if (seenImg.has(id)) continue;
    seenImg.add(id);
    images.push({
      id,
      file: spriteFileHint(id),
      chroma: w.chroma_key !== false,
    });
  }
  if (!images.length && !animations.length) return "";
  const lines = [
    "# Sprite packs are inlined under each sprite as pack: { png, cuts, fps, ... }.",
    "# You can also !include a .yml pack, or keep a legacy image: block.",
    "",
  ];
  if (images.length) {
    lines.push("# Legacy image: stubs if a sheet is not inlined yet:");
    lines.push("image:");
    for (const img of images) {
      lines.push(`  - platform: file`);
      lines.push(`    id: ${img.id}`);
      lines.push(`    file: ${img.file}`);
      lines.push(`    type: RGB565`);
      if (img.chroma) lines.push(`    transparency: chroma_key`);
    }
    lines.push("");
  }
  if (animations.length) {
    lines.push("animation:");
    for (const anim of animations) {
      lines.push(`  - id: ${anim.id}`);
      lines.push(`    file: ${anim.file}`);
    }
    lines.push("");
  }
  return lines.join("\n");
}

export function collectHaImports(state) {
  const seen = new Map();
  const add = (item) => {
    if (!item?.id || !item.entity_id) return;
    const key = `${item.platform}:${item.id}:${item.attribute || ""}`;
    if (!seen.has(key)) seen.set(key, item);
  };
  const addWeather = (w) => {
    const entity = w.ha_entity || exampleById(w.condition_id)?.entity_id;
    if (!w.condition_id || !entity) return;
    add({ id: w.condition_id, entity_id: entity, platform: "text_sensor" });
    if (w.show_temp) {
      add({
        id: w.temperature_id || `${w.condition_id}_temperature`,
        entity_id: entity,
        platform: "sensor",
        attribute: "temperature",
      });
    }
    if (w.show_humidity) {
      add({
        id: w.humidity_id || `${w.condition_id}_humidity`,
        entity_id: entity,
        platform: "sensor",
        attribute: "humidity",
      });
    }
    if (w.show_wind) {
      add({
        id: w.wind_speed_id || `${w.condition_id}_wind_speed`,
        entity_id: entity,
        platform: "sensor",
        attribute: "wind_speed",
      });
      add({
        id: w.wind_bearing_id || `${w.condition_id}_wind_bearing`,
        entity_id: entity,
        platform: "sensor",
        attribute: "wind_bearing",
      });
    }
  };
  for (const w of allWidgets(state)) {
    if (w.type === "weather") {
      addWeather(w);
    } else if (w.ha_entity) {
      const id = w.sensor_id || w.text_sensor_id;
      if (id) {
        add({
          id,
          entity_id: w.ha_entity,
          platform: w.ha_platform || (w.type === "text" && w.source !== "number" ? "text_sensor" : "sensor"),
        });
      }
    } else {
      const id = w.sensor_id || w.text_sensor_id;
      const ex = id ? exampleById(id) : null;
      if (ex) {
        add({
          id: ex.id,
          entity_id: ex.entity_id,
          platform: ex.platform === "text_sensor" ? "text_sensor" : "sensor",
        });
      }
    }
    const vis = normalizeVisible(w.visible);
    for (const rule of vis?.rules || []) {
      if (!rule.ha_entity) continue;
      const yaml = ruleToYaml(rule);
      if (yaml?.sensor_id) add({ id: yaml.sensor_id, entity_id: rule.ha_entity, platform: "sensor" });
      if (yaml?.text_sensor_id) add({ id: yaml.text_sensor_id, entity_id: rule.ha_entity, platform: "text_sensor" });
    }
  }
  const items = [...seen.values()];
  const substitutions = new Map();
  for (const item of items) {
    const key = substitutionKey(item.entity_id);
    item.subst = key;
    if (!substitutions.has(key)) substitutions.set(key, item.entity_id);
  }
  return { items, substitutions };
}

function formatHaSensorsYaml(items, extraSensorLines = []) {
  const sensors = items.filter((x) => x.platform === "sensor");
  const texts = items.filter((x) => x.platform === "text_sensor");
  const lines = [];
  if (extraSensorLines.length || sensors.length) {
    lines.push("sensor:");
    if (extraSensorLines.length) lines.push(...extraSensorLines);
    for (const s of sensors) {
      lines.push(`  - platform: homeassistant`);
      lines.push(`    id: ${s.id}`);
      lines.push(`    entity_id: \${${s.subst}}`);
      if (s.attribute) lines.push(`    attribute: ${s.attribute}`);
      lines.push(`    internal: true`);
    }
    lines.push("");
  }
  if (texts.length) {
    lines.push("text_sensor:");
    for (const s of texts) {
      lines.push(`  - platform: homeassistant`);
      lines.push(`    id: ${s.id}`);
      lines.push(`    entity_id: \${${s.subst}}`);
      lines.push(`    internal: true`);
    }
    lines.push("");
  }
  return lines.join("\n");
}

/** @deprecated Prefer collectHaImports + shared substitutions in toYaml. */
export function haImportsYaml(state) {
  const { items, substitutions } = collectHaImports(state);
  if (!items.length) return "";
  return `${substitutionsYaml(substitutions)}${formatHaSensorsYaml(items)}`;
}

export function widgetsFromDoc(doc) {
  if (!doc || typeof doc !== "object") return [];
  if (Array.isArray(doc.widgets)) return doc.widgets;
  if (Array.isArray(doc.children)) return doc.children;
  const root = doc.root;
  if (root && Array.isArray(root.children)) return root.children;
  if (root && root.type && root.type !== "stack") return [root];
  if (doc.widget && typeof doc.widget === "object") return [doc.widget];
  if (doc.type) return [doc];
  return [];
}

export function layoutFromDoc(doc) {
  if (!doc || typeof doc !== "object") return null;
  if (doc.pixel_layout && typeof doc.pixel_layout === "object") return doc.pixel_layout;
  if (Array.isArray(doc.screens) || doc.root) return doc;
  return null;
}

export function extractCustomPatch(doc) {
  if (!doc || typeof doc !== "object") return null;
  let w = doc;
  if (doc.widget && typeof doc.widget === "object") w = doc.widget;
  const raw = w.pixels ?? doc.pixels;
  const pixels = decodePixelValue(raw, w.width || doc.width, w.height || doc.height);
  if (!pixels.length && w.type !== "custom") return null;
  const patch = {};
  if (pixels.length) patch.pixels = pixels;
  if (Array.isArray(w.palette) || Array.isArray(doc.palette)) patch.palette = w.palette || doc.palette;
  if (w.width) patch.width = Number(w.width);
  if (w.height) patch.height = Number(w.height);
  if (pixels.length) {
    patch.height = pixels.length;
    patch.width = Math.max(...pixels.map((row) => String(row).length), patch.width || 1);
  }
  if (w.color) patch.color = w.color;
  if (w.opacity != null) patch.opacity = w.opacity;
  if (w.animation) patch.animation = { ...w.animation };
  return patch;
}
