"""Tests for pixel_layout SD bundle packing."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from sd_bundle import (
    MANIFEST_NAME,
    PLAYLIST_NAME,
    build_plbundle,
    build_sd_tree,
    export_sd_zip,
    pack_layout,
    parse_plbundle,
    playlist_from_pixel_layout,
)


class SdBundleTest(unittest.TestCase):
    def test_roundtrip_plbundle(self):
        tree = {
            "manifest.json": b'{"schema":1}\n',
            "playlist.yml": b"rotate: 8s\nscreens: []\n",
            "packs/demo.yml": b"format: pixel_layout.sprite/v1\n",
        }
        blob = build_plbundle(tree)
        back = parse_plbundle(blob)
        self.assertEqual(back, tree)

    def test_pack_layout_minimal(self):
        doc = {
            "pixel_layout": {
                "display_id": "matrix",
                "background": "black",
                "rotate": "8s",
                "screens": [
                    {
                        "id": "main",
                        "duration": "10s",
                        "root": {"type": "stack", "children": []},
                    }
                ],
            }
        }
        tree, blob = pack_layout(doc, panel_width=128, panel_height=64)
        self.assertIn(MANIFEST_NAME, tree)
        self.assertIn(PLAYLIST_NAME, tree)
        manifest = json.loads(tree[MANIFEST_NAME].decode())
        self.assertEqual(manifest["panel_width"], 128)
        self.assertEqual(manifest["schema"], 1)
        self.assertEqual(parse_plbundle(blob), tree)

    def test_export_zip(self):
        tree = build_sd_tree({"pixel_layout": {"display_id": "m", "screens": []}})
        z = export_sd_zip(tree)
        self.assertTrue(z[:2] == b"PK")

    def test_playlist_from_nested(self):
        pl = playlist_from_pixel_layout({"pixel_layout": {"display_id": "x"}})
        self.assertEqual(pl["display_id"], "x")


if __name__ == "__main__":
    unittest.main()
