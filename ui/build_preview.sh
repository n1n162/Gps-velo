#!/bin/sh
# Apercu des ecrans sur ordinateur (sans carte) : compile LVGL, rend les pages et simule le toucher.
# Necessite git, gcc, python3 avec Pillow, et dans tests/data : sortie.gpx, replay2.csv (voir tests/make_replay.py),
# et facultativement gps.csv (base de cols) pour afficher les noms de montees.
set -e
cd "$(dirname "$0")"
LVGL_TAG=v9.2.2
[ -d lvgl ] || git clone --depth 1 --branch "$LVGL_TAG" https://github.com/lvgl/lvgl.git lvgl
mkdir -p build/obj
if [ ! -f build/liblvgl.a ]; then
  for f in $(find lvgl/src -name '*.c'); do
    o=build/obj/$(echo "$f" | tr '/' '_' | sed 's/\.c$/.o/')
    [ -f "$o" ] || gcc -O1 -w -I. -Ilvgl -DLV_CONF_INCLUDE_SIMPLE -c "$f" -o "$o"
  done
  ar rcs build/liblvgl.a build/obj/*.o
fi
gcc -O1 -Wall -Wextra -I. -Ilvgl -I../core -DLV_CONF_INCLUDE_SIMPLE ui.c ui_preview.c \
    ../core/cols.c ../core/climb.c ../core/route.c ../core/matcher.c fonts/ui_font_*.c build/liblvgl.a -lm -o build/ui_preview
cd ../tests/data
../../ui/build/ui_preview
python3 ../../ui/rgb565_to_png.py
