# Carte Waveshare ESP32-S3-Touch-LCD-3.5 : notes vérifiées

Sources : dépôt officiel `waveshareteam/ESP32-S3-Touch-LCD-3.5` (licence Apache 2.0) et docs.waveshare.com.

## Vérifié dans le code et la documentation Waveshare
- Processeur ESP32-S3R8 : 8 Mo de PSRAM, 16 Mo de flash. Deux versions matérielles, V1 et V2, qui partagent les mêmes programmes (la V2 ajoute la détection de carte SD et une protection contre l'inversion de batterie). L'étiquette au dos indique la version.
- Écran 320 x 480 (portrait natif), pilote ST7796 en SPI : MOSI GPIO1, SCLK GPIO5, DC GPIO3, rétroéclairage GPIO6 (PWM). CS et RESET ne sont pas affectés à un GPIO dans le code ; l'horloge SPI est à 80 MHz.
- Tactile FT6336 sur le bus I2C partagé : SDA GPIO8, SCL GPIO7.
- Adresses I2C déjà occupées : AXP2101 0x34, FT6336 0x38, PCF85063 0x51, QMI8658 0x6A ou 0x6B, plus l'expandeur TCA9554.
- Carte microSD en SDMMC 1 bit : CMD GPIO10, D0 GPIO9, CLK GPIO11.
- Bouton BOOT sur GPIO0.
- Exemples ESP-IDF 5.4.0 avec LVGL 8.4 et le composant esp_lvgl_port, dont le verrou de dépendances accepte LVGL >= 8 et < 10.

## Non vérifié
- Broches libres de l'en-tête 2,54 mm (pour le GNSS en UART) : le tableau n'existe que sous forme d'image sur la page Waveshare. À confirmer sur l'image "Pinout Definition".
- Compatibilité de l'interface LVGL 9.2 de ce projet avec esp_lvgl_port : à compiler sur la carte.
- Adresses du baromètre : BMP390 et DPS310 sont donnés à 0x76/0x77 dans leurs fiches (à vérifier), donc sans conflit avec la liste ci-dessus.
