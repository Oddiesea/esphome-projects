"""Tests for SD entity id collection in pixel_layout codegen."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from esphome.const import CONF_ID

from entity_ids import collect_registerable_entity_ids


class EntityIdsTest(unittest.TestCase):
    def test_top_level_sensor(self):
        self.assertEqual(
            collect_registerable_entity_ids({CONF_ID: "outdoor_temp", "platform": "template"}),
            ["outdoor_temp"],
        )

    def test_veml7700_nested_ambient_light(self):
        entry = {
            CONF_ID: "veml7700_veml7700component_id",
            "platform": "veml7700",
            "ambient_light": {CONF_ID: "ambient_lux"},
        }
        self.assertEqual(collect_registerable_entity_ids(entry), ["ambient_lux"])

    def test_homeassistant_sensor(self):
        self.assertEqual(
            collect_registerable_entity_ids(
                {CONF_ID: "kaya_poos", "platform": "homeassistant", "entity_id": "input_number.kaya_poos"}
            ),
            ["kaya_poos"],
        )


if __name__ == "__main__":
    unittest.main()
