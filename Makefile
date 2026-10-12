# Tests sur ordinateur du code commun (montees, GPX, positionnement, FIT, noms de cols).
# Il faut un fichier GPX de test avec heures et frequence cardiaque : tests/data/sortie.gpx (non publie, voir README).
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Icore
BUILD = build
DATA = tests/data

.PHONY: all test nommage clean
all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/test_climb: tests/test_climb.c core/climb.c core/route.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/test_cols: tests/test_cols.c core/cols.c core/climb.c core/route.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/test_route: tests/test_route.c core/route.c core/matcher.c core/climb.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/test_fit: tests/test_fit.c core/fit.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@
$(BUILD)/name_real: tests/name_real.c core/cols.c core/climb.c core/route.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@

$(DATA)/sortie.gpx:
	@echo "Place un GPX de test (avec heures et frequence cardiaque) dans $(DATA)/sortie.gpx"; exit 1

$(DATA)/replay.csv $(DATA)/replay2.csv: $(DATA)/sortie.gpx tests/make_replay.py
	cd $(DATA) && python3 ../make_replay.py sortie.gpx

test: $(BUILD)/test_climb $(BUILD)/test_cols $(BUILD)/test_route $(BUILD)/test_fit $(DATA)/replay.csv $(DATA)/replay2.csv
	cd $(DATA) && ../../$(BUILD)/test_climb sortie.gpx
	cd $(DATA) && ../../$(BUILD)/test_cols
	cd $(DATA) && ../../$(BUILD)/test_route sortie.gpx
	cd $(DATA) && ../../$(BUILD)/test_fit
	cd $(DATA) && python3 ../verify_fit.py

# Nommage avec une vraie base de cols (tests/data/gps.csv, export OpenStreetMap converti)
nommage: $(BUILD)/name_real
	cd $(DATA) && ../../$(BUILD)/name_real filtre 0

clean:
	rm -rf $(BUILD) $(DATA)/ride*.fit $(DATA)/replay*.csv
