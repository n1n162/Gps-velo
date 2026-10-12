#!/usr/bin/env python3
"""
Construit gps.csv à partir de tous les fichiers JSON et GeoJSON
présents dans le dossier sources.

Format de sortie :
type;nom;lat;lon;alt

Usage :
python3 construire_gps.py
"""

import json
import math
import re
import sys
from pathlib import Path

DOSSIER = Path(__file__).resolve().parent
DOSSIER_SOURCES = DOSSIER / "sources"
FICHIER_SORTIE = DOSSIER / "gps.csv"

PLACES = {
    "village",
    "hamlet",
    "town",
    "suburb",
    "locality",
    "isolated_dwelling",
}


def parse_ele(value):
    """Convertit l'altitude en entier, si elle est disponible."""
    if value is None:
        return ""

    match = re.search(
        r"-?\d+(?:[.,]\d+)?",
        str(value).replace(" ", "")
    )

    if not match:
        return ""

    try:
        altitude = float(match.group(0).replace(",", "."))
    except ValueError:
        return ""

    if -500 < altitude < 9000:
        return str(round(altitude))

    return ""


def distance(lat1, lon1, lat2, lon2):
    """Distance approximative entre deux coordonnées, en mètres."""
    dy = (lat2 - lat1) * 111320
    dx = (
        (lon2 - lon1)
        * 111320
        * math.cos(math.radians((lat1 + lat2) / 2))
    )

    return math.hypot(dx, dy)


def extraire_points(data):
    """Extrait les points d'un GeoJSON ou d'un JSON Overpass."""

    if isinstance(data, dict) and "features" in data:
        for feature in data["features"]:
            geometry = feature.get("geometry") or {}

            if geometry.get("type") != "Point":
                continue

            coordinates = geometry.get("coordinates", [])

            if len(coordinates) < 2:
                continue

            lon, lat = coordinates[:2]

            yield feature.get("properties") or {}, lat, lon

    elif isinstance(data, dict) and "elements" in data:
        for element in data["elements"]:
            if element.get("type") != "node":
                continue

            if "lat" not in element or "lon" not in element:
                continue

            yield (
                element.get("tags") or {},
                element["lat"],
                element["lon"]
            )

    else:
        print("Attention : format JSON non reconnu.")
        return


def classifier(tags):
    """Détermine la catégorie d'un point à partir de ses attributs OSM."""

    if (
        tags.get("mountain_pass") == "yes"
        or tags.get("natural") == "saddle"
    ):
        return "col"

    if tags.get("place") in PLACES:
        return "lieu"

    # Catégories supplémentaires pour de futurs exports OSM.
    if tags.get("amenity") == "drinking_water":
        return "eau"

    if tags.get("man_made") == "water_tap":
        return "eau"

    if tags.get("tourism") == "alpine_hut":
        return "refuge"

    if tags.get("amenity") == "shelter":
        return "abri"

    if tags.get("natural") == "peak":
        return "sommet"

    return None


def lire_fichier(fichier):
    """Lit un fichier et renvoie les points reconnus."""
    points = []
    ignores = 0

    try:
        with fichier.open("r", encoding="utf-8-sig") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError) as erreur:
        print(f"ERREUR : lecture de {fichier.name} : {erreur}")
        return points, 0, 1

    try:
        for tags, lat, lon in extraire_points(data):
            type_point = classifier(tags)

            if type_point is None:
                ignores += 1
                continue

            nom = (tags.get("name") or "").strip()

            if not nom:
                ignores += 1
                continue

            try:
                lat = float(lat)
                lon = float(lon)

                if not (
                    math.isfinite(lat)
                    and math.isfinite(lon)
                    and -90 <= lat <= 90
                    and -180 <= lon <= 180
                ):
                    ignores += 1
                    continue

            except (TypeError, ValueError):
                ignores += 1
                continue

            points.append([
                type_point,
                nom.replace(";", ","),
                round(lat, 6),
                round(lon, 6),
                parse_ele(tags.get("ele"))
            ])

    except (TypeError, ValueError, KeyError, IndexError) as erreur:
        print(f"ERREUR : données de {fichier.name} : {erreur}")
        return [], ignores, 1

    return points, ignores, 0


def fusionner_doublons_cols(points):
    """Fusionne les cols de même nom situés à moins de 300 mètres."""

    cols = [p for p in points if p[0] == "col"]
    autres = [p for p in points if p[0] != "col"]

    conserves = []
    doublons = 0

    for point in cols:
        doublon = next(
            (
                existant for existant in conserves
                if existant[1].casefold() == point[1].casefold()
                and distance(
                    existant[2], existant[3],
                    point[2], point[3]
                ) < 300
            ),
            None
        )

        if doublon is None:
            conserves.append(point)
        else:
            doublons += 1

            if not doublon[4] and point[4]:
                doublon[4] = point[4]

    return conserves + autres, doublons


def main():
    if not DOSSIER_SOURCES.is_dir():
        print(f"Dossier sources introuvable : {DOSSIER_SOURCES}")
        sys.exit(1)

    fichiers = sorted(
        fichier
        for fichier in DOSSIER_SOURCES.iterdir()
        if fichier.is_file()
        and fichier.suffix.lower() in {".json", ".geojson"}
    )

    if not fichiers:
        print("Aucun fichier JSON ou GeoJSON dans le dossier sources.")
        sys.exit(1)

    points = []
    total_ignores = 0
    erreurs = 0

    print("Lecture des fichiers :")

    for fichier in fichiers:
        print(f"  {fichier.name}")

        resultat, ignores, nb_erreurs = lire_fichier(fichier)

        points.extend(resultat)
        total_ignores += ignores
        erreurs += nb_erreurs

        print(f"    Points reconnus : {len(resultat)}")

    # Ne pas remplacer une base existante si un fichier est illisible.
    if erreurs:
        print(
            "\nConstruction annulée : au moins un fichier est illisible. "
            "Corrige les erreurs puis relance le script."
        )
        sys.exit(1)

    points, doublons = fusionner_doublons_cols(points)

    points.sort(
        key=lambda p: (p[2], p[3], p[0], p[1].casefold())
    )

    try:
        with FICHIER_SORTIE.open(
            "w", encoding="utf-8", newline="\n"
        ) as f:
            f.write(
                "# type;nom;lat;lon;alt - donnees (c) "
                "les contributeurs d'OpenStreetMap, licence ODbL\n"
            )

            for type_point, nom, lat, lon, alt in points:
                f.write(
                    f"{type_point};{nom};{lat:.6f};"
                    f"{lon:.6f};{alt}\n"
                )

    except OSError as erreur:
        print(f"Erreur d'écriture de gps.csv : {erreur}")
        sys.exit(1)

    categories = {}

    for point in points:
        categories[point[0]] = categories.get(point[0], 0) + 1

    print("\nConstruction terminée.")
    print(f"Fichier : {FICHIER_SORTIE}")
    print(f"Total des points : {len(points)}")
    print(f"Doublons de cols fusionnés : {doublons}")
    print(f"Éléments ignorés : {total_ignores}")

    print("\nRépartition par catégorie :")

    for categorie, nombre in sorted(categories.items()):
        print(f"  {categorie} : {nombre}")


if __name__ == "__main__":
    main()
