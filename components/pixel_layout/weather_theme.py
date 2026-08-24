"""Weather icon themes (custom pixel art per condition)."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml

THEMES_DIR = Path(__file__).with_name("weather_themes")

# Canonical icon keys + aliases expanded at load time.
WEATHER_ICON_ALIASES: dict[str, str] = {
    "clear": "sunny",
    "overcast": "cloudy",
    "partly-cloudy": "partlycloudy",
    "partly-cloudy-day": "partlycloudy",
    "partly-cloudy-night": "partlycloudy",
    "rain": "rainy",
    "snow": "snowy",
    "snowy-rainy": "snowy",
    "lightning-rainy": "lightning",
    "thunder": "lightning",
    "foggy": "fog",
    "mist": "fog",
    "windy-variant": "windy",
}

CANONICAL_WEATHER_ICON_KEYS = (
    "sunny",
    "clear-night",
    "cloudy",
    "partlycloudy",
    "rainy",
    "pouring",
    "snowy",
    "hail",
    "lightning",
    "fog",
    "windy",
    "exceptional",
    "default",
)


def normalize_weather_key(condition: str | None) -> str:
    if not condition:
        return "cloudy"
    key = str(condition).strip().lower()
    if key.startswith("mdi:weather-"):
        key = key[12:]
    elif key.startswith("weather-"):
        key = key[8:]
    key = key.replace("_", "-").replace(" ", "-")
    while "--" in key:
        key = key.replace("--", "-")
    return key.strip("-") or "cloudy"


def resolve_weather_icon_key(key: str) -> str:
    key = normalize_weather_key(key)
    return WEATHER_ICON_ALIASES.get(key, key)


def list_weather_themes() -> list[str]:
    if not THEMES_DIR.is_dir():
        return []
    return sorted(p.stem for p in THEMES_DIR.glob("*.yml"))


def load_weather_theme(theme_id: str) -> dict[str, Any]:
    stem = theme_id
    path = THEMES_DIR / f"{stem}.yml"
    if not path.is_file() and stem == "mario":
        stem = "gameman"
        path = THEMES_DIR / f"{stem}.yml"
    if not path.is_file():
        raise FileNotFoundError(f"Unknown weather icon theme: {theme_id}")
    doc = yaml.safe_load(path.read_text()) or {}
    icons: dict[str, Any] = {}
    raw = doc.get("icons") or {}
    for name, spec in raw.items():
        if not isinstance(spec, dict):
            continue
        icons[normalize_weather_key(name)] = spec
    # Expand aliases to canonical entries when missing.
    for alias, target in WEATHER_ICON_ALIASES.items():
        alias = normalize_weather_key(alias)
        target = normalize_weather_key(target)
        if alias not in icons and target in icons:
            icons[alias] = icons[target]
    expanded: dict[str, Any] = {}
    for key, spec in icons.items():
        expanded[key] = spec
        canon = resolve_weather_icon_key(key)
        if canon not in expanded:
            expanded[canon] = spec
    return {
        "id": str(doc.get("id") or theme_id),
        "name": str(doc.get("name") or theme_id),
        "size": int(doc.get("size") or 16),
        "icons": expanded,
    }


def merge_weather_icons(config: dict[str, Any]) -> dict[str, Any]:
    """Return icons map from icon_theme + inline icons overrides."""
    icons: dict[str, Any] = {}
    theme_id = config.get("icon_theme")
    if theme_id:
        theme = load_weather_theme(str(theme_id))
        icons.update(theme.get("icons") or {})
    inline = config.get("icons") or {}
    if isinstance(inline, dict):
        for name, spec in inline.items():
            if isinstance(spec, dict):
                icons[normalize_weather_key(name)] = spec
    return icons
