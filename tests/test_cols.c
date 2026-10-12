#include <stdio.h>
#include <string.h>
#include "cols.h"

typedef struct { FILE *f; } fctx;
static size_t rd(void *c, char *b, size_t m) { return fread(b, 1, m, ((fctx *)c)->f); }
typedef struct { const char *s; size_t pos; } mctx;
static size_t rd_mem(void *c, char *b, size_t m) { mctx *x = c; size_t n = strlen(x->s + x->pos); if (n > m) n = m; memcpy(b, x->s + x->pos, n); x->pos += n; return n; }

static int fails;
static void check(const char *label, const char *got, const char *want)
{
    int ok = strcmp(got, want) == 0;
    printf("  %-34s : %-28s %s\n", label, got[0] ? got : "(sans nom)", ok ? "OK" : "ECHEC (attendu : ");
    if (!ok) { printf("%s)\n", want[0] ? want : "(sans nom)"); fails++; }
}

int main(void)
{
    /* 1. grammaire des noms de lieux */
    printf("GRAMMAIRE\n");
    struct { const char *in, *want; } g[] = {
        {"Burcin", "Mont\xC3\xA9" "e de Burcin"}, {"Le Revel", "Mont\xC3\xA9" "e du Revel"}, {"La B\xC3\xA2thie", "Mont\xC3\xA9" "e de la B\xC3\xA2thie"},
        {"Les Abrets", "Mont\xC3\xA9" "e des Abrets"}, {"L'Isle", "Mont\xC3\xA9" "e de l'Isle"}, {"Arbin", "Mont\xC3\xA9" "e d'Arbin"},
        {"\xC3\x89tang", "Mont\xC3\xA9" "e d'\xC3\x89tang"}, {"Hauteville", "Mont\xC3\xA9" "e de Hauteville"},
    };
    for (unsigned i = 0; i < sizeof g / sizeof g[0]; i++) { char b[64]; cols_montee_de(g[i].in, b, sizeof b); check(g[i].in, b, g[i].want); }

    /* 2. lecture du CSV : fins de ligne Windows, altitude absente, lignes parasites */
    printf("LECTURE CSV\n");
    static cols_entry_t e1[16]; static char p1[1024]; cols_db_t d1; cols_init(&d1, e1, 16, p1, sizeof p1);
    mctx m = { "type;nom;lat;lon;alt\r\n# commentaire\r\ncol;Col de la Croix-Haute;44.6;5.7;1179\r\nlieu;Saint-\xC3\x89tienne;45.1;5.2;\r\nn'importe quoi\r\ncol;Sans coordonnees;;;\r\ncol;Dernier;45.0;5.0;900", 0 };
    int n = cols_load(rd_mem, &m, &d1);
    printf("  %d entrees lues (attendu 3), %s | %s | %s\n", n, cols_name(&d1, 0), cols_name(&d1, 1), cols_name(&d1, 2));
    if (n != 3 || d1.e[1].ele != COLS_NO_ELE || d1.e[0].ele != 1179) { puts("  ECHEC"); fails++; } else puts("  OK");

    /* 3. nommage des montees de ta sortie */
    printf("NOMMAGE DES MONTEES (base de test)\n");
    fctx cx = { fopen("sortie.gpx", "rb") };
    static float X[20000], Y[20000], E[20000], W[20000];
    route_t r; route_init(&r, X, Y, E, 20000, 10.0f); gpx_load(rd, &cx, &r); fclose(cx.f);
    climb_cfg_t cfg; climb_cfg_default(&cfg); profile_smooth(E, r.n, cfg.smooth_half, W);
    static climb_t cl[64]; int k = climb_detect(E, r.n, W, &cfg, cl, 64);

    static cols_entry_t ent[4000]; static char pool[100000]; cols_db_t db; cols_init(&db, ent, 4000, pool, sizeof pool);
    fctx cf = { fopen("cols_fixture.csv", "rb") };
    int ne = cols_load(rd, &cf, &db); fclose(cf.f);
    printf("  base : %d entrees\n", ne);
    cols_cfg_t cc; cols_cfg_default(&cc);
    int named = cols_name_climbs(&db, &cc, &r, E, cl, k);
    const char *want[7] = {"Col du Mont Tournier", "Mont\xC3\xA9" "e du Collet", "", "Mont\xC3\xA9" "e des Abrets", "", "", "Mont\xC3\xA9" "e de Burcin"};
    for (int i = 0; i < k && i < 7; i++) { char lab[40]; snprintf(lab, sizeof lab, "montee %d (%.1f km, %.1f %%)", i + 1, (double)(cl[i].length_m / 1000.0f), (double)cl[i].avg_grade_pct); check(lab, cl[i].name, want[i]); }
    printf("  %d montees nommees sur %d\n", named, k);
    printf("%s\n", fails ? "ECHECS" : "TOUS LES TESTS REUSSIS");
    return fails != 0;
}
