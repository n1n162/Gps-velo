#!/usr/bin/env python3
"""Genere replay.csv et replay2.csv a partir d'un GPX enregistre (avec heure, et frequence cardiaque si disponible).
Usage (depuis tests/data) : python3 ../make_replay.py sortie.gpx
  replay.csv  : lat, lon, distance vraie, vitesse, cap        (test du positionnement)
  replay2.csv : heure, lat, lon, alt, dist, vitesse, FC, cadence, puissance   (tests FIT et ecrans)
La cadence est synthetique (elle n'existe pas dans le GPX) ; la puissance est absente (-1)."""
import re, math, datetime, sys
src = sys.argv[1] if len(sys.argv) > 1 else "sortie.gpx"
txt = open(src, encoding="utf-8").read()
pat = r'<trkpt lat="([-\d.]+)" lon="([-\d.]+)">\s*<ele>([-\d.]+)</ele>\s*<time>([^<]+)</time>(.*?)</trkpt>'
pts = re.findall(pat, txt, flags=re.S)
if len(pts) < 2: raise SystemExit("GPX sans points avec altitude et heure")
R = 6371008.8
def hav(a, b, c, d):
    p1, p2 = math.radians(a), math.radians(c)
    h = math.sin((p2 - p1) / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(math.radians(d - b) / 2) ** 2
    return 2 * R * math.asin(math.sqrt(h))
def bearing(a, b, c, d):
    p1, p2 = math.radians(a), math.radians(c); dl = math.radians(d - b)
    y = math.sin(dl) * math.cos(p2); x = math.cos(p1) * math.sin(p2) - math.sin(p1) * math.cos(p2) * math.cos(dl)
    return (math.degrees(math.atan2(y, x)) + 360) % 360
la = [float(p[0]) for p in pts]; lo = [float(p[1]) for p in pts]; el = [float(p[2]) for p in pts]
tm = [int(datetime.datetime.strptime(p[3], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=datetime.timezone.utc).timestamp()) for p in pts]
hr = []
for p in pts:
    m = re.search(r"<gpxtpx:hr>(\d+)</gpxtpx:hr>", p[4]); hr.append(int(m.group(1)) if m else -1)
d = [0.0]
for i in range(1, len(la)): d.append(d[-1] + hav(la[i-1], lo[i-1], la[i], lo[i]))
sp = [0.0] + [((d[i] - d[i-1]) / (tm[i] - tm[i-1]) if tm[i] > tm[i-1] else 0.0) for i in range(1, len(la))]
with open("replay.csv", "w") as f:
    for i in range(len(la)):
        j = i + 1 if i + 1 < len(la) else i - 1
        seg = hav(la[i], lo[i], la[j], lo[j]) if j != i else 0
        hd = bearing(la[i], lo[i], la[j], lo[j]) if seg >= 1.0 and j > i else (bearing(la[j], lo[j], la[i], lo[i]) if seg >= 1.0 else -1)
        f.write("%.7f,%.7f,%.3f,%.2f,%.1f\n" % (la[i], lo[i], d[i], sp[i], hd))
with open("replay2.csv", "w") as f:
    for i in range(len(la)):
        cad = int(70 + 15 * math.sin(i / 300.0)) if sp[i] > 1.0 else 0
        f.write("%d,%.7f,%.7f,%.1f,%.2f,%.3f,%d,%d,-1\n" % (tm[i], la[i], lo[i], el[i], d[i], sp[i], hr[i], cad))
print("%d points, %.1f km, FC %s" % (len(la), d[-1] / 1000, "oui" if max(hr) >= 0 else "non (les tests FIT supposent une FC)"))
