#!/usr/bin/env python3
"""Convertit les captures screen_*.rgb565 du dossier courant en PNG (agrandies 2 fois). Necessite Pillow."""
import glob
from PIL import Image
for path in sorted(glob.glob("screen_*.rgb565")):
    raw = open(path, "rb").read(); px = bytearray()
    for i in range(0, len(raw), 2):
        v = raw[i] | (raw[i + 1] << 8)
        px += bytes((((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31))
    out = path.replace("screen_", "apercu_").replace(".rgb565", ".png")
    Image.frombytes("RGB", (320, 480), bytes(px)).resize((640, 960), Image.NEAREST).save(out)
    print(out)
