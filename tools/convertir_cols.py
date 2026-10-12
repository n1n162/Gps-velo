#!/usr/bin/env python3
"""Convertit un export OpenStreetMap (GeoJSON d'Overpass Turbo, ou JSON brut d'Overpass)
en fichier de base pour le GPS : type;nom;lat;lon;alt

Usage : python3 convertir_cols.py export.geojson cols.csv
Les cols sont les points mountain_pass=yes ou natural=saddle ; les lieux sont les points place=village, hamlet, town,
suburb ou locality. Donnees (c) les contributeurs d'OpenStreetMap, licence ODbL."""
import json, math, re, sys

PLACES = {"village", "hamlet", "town", "suburb", "locality", "isolated_dwelling"}

def parse_ele(v):
    if v is None: return ""
    m = re.search(r"-?\d+(?:[.,]\d+)?", str(v).replace(" ", ""))
    if not m: return ""
    x = float(m.group(0).replace(",", "."))
    return str(int(round(x))) if -500 < x < 9000 else ""

def items(data):
    if isinstance(data, dict) and "features" in data:
        for f in data["features"]:
            g = f.get("geometry") or {}
            if g.get("type") != "Point": continue
            lon, lat = g["coordinates"][:2]
            yield (f.get("properties") or {}), lat, lon
    elif isinstance(data, dict) and "elements" in data:
        for e in data["elements"]:
            if e.get("type") == "node" and "lat" in e:
                yield (e.get("tags") or {}), e["lat"], e["lon"]
    else:
        raise SystemExit("Format non reconnu : attendu un GeoJSON ou un JSON Overpass")

def metres(la1, lo1, la2, lo2):
    dy = (la2 - la1) * 111320.0
    dx = (lo2 - lo1) * 111320.0 * math.cos(math.radians((la1 + la2) / 2))
    return math.hypot(dx, dy)

def main():
    if len(sys.argv) != 3: raise SystemExit(__doc__)
    data = json.load(open(sys.argv[1], encoding="utf-8"))
    rows, skipped = [], 0
    for tags, lat, lon in items(data):
        name = (tags.get("name") or "").strip().replace(";", ",")
        if not name: skipped += 1; continue
        if tags.get("mountain_pass") == "yes" or tags.get("natural") == "saddle": kind = "col"
        elif tags.get("place") in PLACES: kind = "lieu"
        else: skipped += 1; continue
        rows.append([kind, name, round(lat, 6), round(lon, 6), parse_ele(tags.get("ele"))])
    # doublons de cols : meme nom a moins de 300 m, on garde l'entree qui a une altitude
    cols = [r for r in rows if r[0] == "col"]
    keep, dropped = [], 0
    for r in cols:
        dup = next((k for k in keep if k[1] == r[1] and metres(k[2], k[3], r[2], r[3]) < 300), None)
        if dup is None: keep.append(r)
        else:
            dropped += 1
            if not dup[4] and r[4]: dup[4] = r[4]
    out = keep + [r for r in rows if r[0] == "lieu"]
    out.sort(key=lambda r: (r[2], r[3]))
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as f:
        f.write("# type;nom;lat;lon;alt - donnees (c) les contributeurs d'OpenStreetMap, licence ODbL\n")
        for r in out: f.write("%s;%s;%s;%s;%s\n" % tuple(r))
    print("%d cols, %d lieux ecrits (%d doublons de cols fusionnes, %d elements ignores)" % (
        sum(1 for r in out if r[0] == "col"), sum(1 for r in out if r[0] == "lieu"), dropped, skipped))

main()
