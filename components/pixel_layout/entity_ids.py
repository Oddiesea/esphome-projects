"""Collect entity ids from ESPHome platform config entries for SD runtime lookup."""

from __future__ import annotations

from esphome.const import CONF_ID


def collect_registerable_entity_ids(entry: dict) -> list:
    """Entity ids from a platform config entry.

    Component platforms (e.g. veml7700) get an auto-generated component id at the
    top level but expose entities in nested blocks (``ambient_light.id``). When
    nested ids exist, skip the top-level component id.
    """
    nested: list = []

    def walk(node) -> None:
        if isinstance(node, dict):
            for key, val in node.items():
                if key == CONF_ID:
                    continue
                if isinstance(val, dict):
                    eid = val.get(CONF_ID)
                    if eid is not None:
                        nested.append(eid)
                    walk(val)
                elif isinstance(val, list):
                    for item in val:
                        walk(item)

    walk(entry)
    if nested:
        return nested
    top = entry.get(CONF_ID)
    return [top] if top is not None else []
