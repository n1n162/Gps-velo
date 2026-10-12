# Base de cols pour nommer les montées

## Fichier `cols.csv` (à copier à la racine de la carte SD)
Une ligne par entrée, séparateur `;`, UTF-8 :

    type;nom;lat;lon;alt
    col;Col du Mont Tournier;45.634886;5.709346;821
    lieu;Burcin;45.457859;5.450367;

`type` vaut `col` ou `lieu` ; `alt` peut être vide. Les lignes qui commencent par `#` sont ignorées.

## Règles de correspondance (réglables dans `cols_cfg_t`)
1. Col : le plus proche du sommet de la montée, à moins de 300 m et à moins de 40 m d'altitude d'écart. Les sources d'altitude diffèrent de 15 à 20 m, d'où cette tolérance.
2. Sinon, lieu : le plus proche du **sommet**, à moins de 1500 m, donnant "Montée du Collet" (avec du, de la, des, de l', d'). Le réglage `place_mode` permet aussi le lieu du départ ("Montée du Revel") ou les deux ("Le Revel > Le Collet").
3. Sinon, pas de nom : l'écran affiche "Montée 3".
4. Pour économiser la mémoire, `cols_filter_route` ne garde que les entrées à moins de 3 km du parcours (1 113 entrées au lieu de 48 033 sur la sortie de test).

## Construire la base à partir d'OpenStreetMap (une seule fois)
1. Ouvre Overpass Turbo, zoome sur la région voulue (par exemple un département à la fois si la zone est grande).
2. Colle cette requête et lance-la :

       [out:json][timeout:180];
       (
         node["mountain_pass"="yes"]["name"]({{bbox}});
         node["natural"="saddle"]["name"]({{bbox}});
         node["place"~"^(village|hamlet|town|suburb|locality)$"]["name"]({{bbox}});
       );
       out body;

3. Exporte le résultat en GeoJSON.
4. Convertis : `python3 convertir_cols.py export.geojson cols.csv` (ou envoie-moi l'export).

La structure de la requête vient d'un exemple publié sur le forum d'OpenStreetMap, mais je n'ai pas pu l'exécuter moi-même : vérifie le nombre de cols obtenu.

## À savoir
- `cols_fixture.csv` est une base de test fabriquée à partir de ta sortie, utilisée seulement par `test_cols.c`. La vraie base est ton export (`gps.csv`).
- Un petit col absent d'OpenStreetMap restera sans nom.
- Données © les contributeurs d'OpenStreetMap, licence ODbL : le crédit doit apparaître dans l'appareil ou sa documentation.
