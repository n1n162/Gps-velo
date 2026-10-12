# -*- coding: utf-8 -*-

import json
import re
import time
import urllib.parse
import urllib.request
import urllib.error
from pathlib import Path

# ============================================================
# CONFIGURATION
# ============================================================

SERVEUR = "https://api-overpass.pikamap.fr/api/interpreter"
DELAI = 180

DOSSIER = Path(__file__).resolve().parent
DOSSIER_SOURCES = DOSSIER / "sources"


# ============================================================
# CATEGORIES DISPONIBLES
# ============================================================

CATEGORIES = {
    "1": {
        "nom": "Cols et passages de montagne",
        "fichier": "cols",
        "requete": [
            'node["mountain_pass"="yes"]["name"](area.departement);',
            'node["natural"="saddle"]["name"](area.departement);'
        ]
    },

    "2": {
        "nom": "Sommets",
        "fichier": "sommets",
        "requete": [
            'node["natural"="peak"]["name"](area.departement);'
        ]
    },

    "3": {
        "nom": "Villes, villages, hameaux et lieux-dits",
        "fichier": "lieux",
        "requete": [
            'node["place"~"^(city|town|village|hamlet|suburb|locality|isolated_dwelling)$"]["name"](area.departement);'
        ]
    },

    "4": {
        "nom": "Eau potable et robinets",
        "fichier": "eau",
        "requete": [
            'node["amenity"="drinking_water"](area.departement);',
            'node["man_made"="water_tap"](area.departement);'
        ]
    },

    "5": {
        "nom": "Sources naturelles",
        "fichier": "sources",
        "requete": [
            'nwr["natural"="spring"](area.departement);'
        ]
    },

    "6": {
        "nom": "Refuges de montagne",
        "fichier": "refuges",
        "requete": [
            'nwr["tourism"="alpine_hut"](area.departement);'
        ]
    },

    "7": {
        "nom": "Abris",
        "fichier": "abris",
        "requete": [
            'nwr["amenity"="shelter"](area.departement);'
        ]
    },

    "8": {
        "nom": "Toilettes publiques",
        "fichier": "toilettes",
        "requete": [
            'nwr["amenity"="toilets"](area.departement);'
        ]
    },

    "9": {
        "nom": "Commerces alimentaires",
        "fichier": "commerces",
        "requete": [
            'nwr["shop"~"^(supermarket|convenience|bakery|butcher|greengrocer)$"](area.departement);'
        ]
    },

    "10": {
        "nom": "Fontaines",
        "fichier": "fontaines",
        "requete": [
            'nwr["amenity"="fountain"](area.departement);'
        ]
    },

    "11": {
        "nom": "Parkings",
        "fichier": "parkings",
        "requete": [
            'nwr["amenity"="parking"](area.departement);'
        ]
    },

    "12": {
        "nom": "Magasins et réparateurs de vélos",
        "fichier": "velocistes",
        "requete": [
            'nwr["shop"="bicycle"](area.departement);',
            'nwr["craft"="bicycle_repair"](area.departement);'
        ]
    },

    "13": {
        "nom": "Gares",
        "fichier": "gares",
        "requete": [
            'node["railway"="station"](area.departement);',
            'node["public_transport"="station"](area.departement);'
        ]
    }
}


# ============================================================
# FONCTIONS
# ============================================================

def nettoyer_nom(nom):
    nom = nom.lower()
    nom = re.sub(r"[^a-z0-9]+", "_", nom)
    return nom.strip("_")


def construire_requete(code, clauses):
    """
    Construit une requête limitée à un département.
    Le code correspond à la référence administrative OSM.
    """

    return (
        f"[out:json][timeout:{DELAI}];\n"
        f'area["boundary"="administrative"]'
        f'["admin_level"="6"]["ref"="{code}"]'
        "->.departement;\n"
        "(\n"
        + "\n".join(clauses)
        + "\n);\n"
        "out center tags;\n"
    )


def interroger_overpass(requete):
    donnees = urllib.parse.urlencode({
        "data": requete
    }).encode("utf-8")

    demande = urllib.request.Request(
        SERVEUR,
        data=donnees,
        headers={
            "User-Agent": "GPSVelo-Downloader/1.0",
            "Content-Type": "application/x-www-form-urlencoded"
        },
        method="POST"
    )

    with urllib.request.urlopen(
        demande,
        timeout=DELAI + 60
    ) as reponse:
        return json.loads(
            reponse.read().decode("utf-8")
        )


def convertir_geojson(resultat):
    """
    Convertit les résultats Overpass en GeoJSON Point.
    Pour les chemins et relations, utilise leur centre approximatif.
    Conserve les tags et l'identifiant OSM.
    """

    features = []
    identifiants_vus = set()

    for element in resultat.get("elements", []):

        type_osm = element.get("type")
        identifiant = element.get("id")

        if type_osm is None or identifiant is None:
            continue

        cle = f"{type_osm}/{identifiant}"

        if cle in identifiants_vus:
            continue

        identifiants_vus.add(cle)

        # Coordonnées des nœuds
        if "lat" in element and "lon" in element:
            latitude = element["lat"]
            longitude = element["lon"]

        # Centre des chemins et relations
        elif "center" in element:
            latitude = element["center"].get("lat")
            longitude = element["center"].get("lon")

        else:
            continue

        if latitude is None or longitude is None:
            continue

        proprietes = dict(element.get("tags", {}))

        # Identifiant unique OpenStreetMap
        proprietes["@id"] = cle
        proprietes["@type"] = type_osm

        features.append({
            "type": "Feature",
            "geometry": {
                "type": "Point",
                "coordinates": [longitude, latitude]
            },
            "properties": proprietes
        })

    return {
        "type": "FeatureCollection",
        "features": features
    }


def telecharger(code, nom_departement, categorie):
    """
    Télécharge une catégorie pour un département.
    Réessaie jusqu'à trois fois en cas d'échec.
    """

    nom_fichier = (
        f"{code}_{nettoyer_nom(nom_departement)}_"
        f"{categorie['fichier']}.geojson"
    )

    chemin = DOSSIER_SOURCES / nom_fichier

    requete = construire_requete(
        code,
        categorie["requete"]
    )

    for tentative in range(1, 4):

        try:
            print(
                f"\nTéléchargement : {nom_departement} "
                f"- {categorie['nom']}"
            )

            resultat = interroger_overpass(requete)

            geojson = convertir_geojson(resultat)

            with chemin.open("w", encoding="utf-8") as fichier:
                json.dump(
                    geojson,
                    fichier,
                    ensure_ascii=False,
                    indent=2
                )

            nombre = len(geojson["features"])

            print(f"  OK : {nombre} objet(s)")
            print(f"  Fichier : {nom_fichier}")

            return True

        except Exception as erreur:
            print(
                f"  Échec {tentative}/3 : {erreur}"
            )

            if tentative < 3:
                attente = tentative * 10
                print(
                    f"  Nouvelle tentative dans "
                    f"{attente} secondes..."
                )
                time.sleep(attente)

    print("  Échec définitif de cette requête.")
    return False


# ============================================================
# PROGRAMME PRINCIPAL
# ============================================================

def main():

    print("=" * 55)
    print("       TELECHARGEMENT DES DONNEES GPS VELO")
    print("=" * 55)

    print("\nDépartements français")
    print("---------------------")
    print("Indique le code du département.")
    print("Exemples : 38 = Isère, 73 = Savoie, 74 = Haute-Savoie.")
    print("Pour plusieurs départements : 38,73,74")

    reponse = input("\nDépartement(s) : ").strip()

    codes = list(dict.fromkeys(
        code.strip().upper()
        for code in reponse.split(",")
        if code.strip()
    ))

    if not codes:
        print("Aucun département sélectionné.")
        return

    print("\nCatégories disponibles")
    print("----------------------")

    for numero, categorie in CATEGORIES.items():
        print(f"{numero:>2}. {categorie['nom']}")

    print("\nIndique les numéros séparés par des virgules.")
    print("Exemple : 1,2,3,4")
    print("Tape T pour toutes les catégories.")

    choix = input("\nCatégories : ").strip().upper()

    if choix in ("T", "TOUT", "TOUTES", "*"):
        categories_selectionnees = list(CATEGORIES.values())

    else:
        numeros = list(dict.fromkeys(
            numero.strip()
            for numero in choix.split(",")
            if numero.strip()
        ))

        invalides = [
            numero for numero in numeros
            if numero not in CATEGORIES
        ]

        if invalides or not numeros:
            print("Sélection de catégories invalide.")
            return

        categories_selectionnees = [
            CATEGORIES[numero] for numero in numeros
        ]

    DOSSIER_SOURCES.mkdir(
        parents=True,
        exist_ok=True
    )

    total = len(codes) * len(categories_selectionnees)

    print("\nRécapitulatif")
    print("-------------")
    print(f"Départements : {', '.join(codes)}")
    print(
        "Catégories : "
        + ", ".join(
            categorie["nom"]
            for categorie in categories_selectionnees
        )
    )
    print(f"Nombre de requêtes : {total}")
    print(f"Dossier de sortie : {DOSSIER_SOURCES}")

    print(
        "\nLes fichiers existants portant le même nom "
        "seront remplacés."
    )

    confirmation = input(
        "\nLancer les téléchargements ? (O/N) : "
    ).strip().upper()

    if confirmation not in ("O", "OUI"):
        print("Téléchargement annulé.")
        return

    termine = 0
    reussies = 0

    for code in codes:

        # Le code administratif sert aussi de nom de département
        # pour le nom du fichier : cela évite les erreurs de nom.
        for categorie in categories_selectionnees:

            termine += 1

            print(f"\n[{termine}/{total}]")

            ok = telecharger(
                code,
                f"dep_{code}",
                categorie
            )

            if ok:
                reussies += 1

            # Pause entre les requêtes pour ménager le serveur.
            if termine < total:
                time.sleep(3)

    print("\n" + "=" * 55)
    print("TÉLÉCHARGEMENTS TERMINÉS")
    print(f"Requêtes réussies : {reussies}/{total}")
    print(f"Dossier : {DOSSIER_SOURCES}")
    print("=" * 55)

    print(
        "\nAttention : vérifie qu'un département a bien été "
        "trouvé si un export contient zéro objet."
    )

    print(
        "\nQuand tu voudras reconstruire ta base GPS, "
        "vérifie que construire_gps.py sait lire les "
        "catégories téléchargées."
    )


if __name__ == "__main__":
    main()