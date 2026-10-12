# Avancement (12 octobre 2026)

## Les cinq exigences
- **Profil des montées type ClimbPro** : fait. Détection avec fusion et recul du départ, catégories Garmin, noms de cols, écrans dédiés. Vérifié sur une sortie de 103 km, comparé à Strava et au profil officiel du Mont Tournier.
- **Enregistrement de la séance** : écriture FIT faite et relue sans erreur par le décodeur officiel Garmin et par fitdecode, avec récupération après coupure. L'acquisition GNSS reste à faire.
- **Export Strava** : fichier prêt. L'envoi automatique n'est pas écrit, rien n'a été testé chez Strava.
- **Guidage sur carte** : trace GPX faite (chargement, positionnement, erreur moyenne de 3 m en simulation). Il manque le fond de carte et les indications de virage.
- **Capteurs FC et cadence** : pas commencé. L'exemple Espressif adapté à une ceinture cardio est repéré.

## Points de vigilance
- Tout est validé sur ordinateur uniquement.
- Les broches libres de l'en-tête de la carte pour le GNSS ne sont pas confirmées (tableau en image sur la page Waveshare).
- Compatibilité de l'interface LVGL 9 avec `esp_lvgl_port` : à confirmer à la compilation sur la carte.
- Toucher sous la pluie ou avec des gants, et lisibilité au soleil : à tester sur le vélo.
- L'altitude diffère de 15 à 20 m selon les sources (dénivelé et noms de cols).
- Les noms de montées viennent du lieu OpenStreetMap le plus proche du sommet : pas toujours l'usage local.

## Prochaines étapes
1. Commander le matériel (carte, GNSS, baromètre, batterie).
2. Projet ESP-IDF : écran et tactile, puis GNSS, enregistrement FIT sur SD, ceinture cardio.
3. Mise à jour en direct des écrans et changement de page automatique.
4. Fond de carte hors ligne.
