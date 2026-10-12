#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "cols.h"
typedef struct { FILE *f; } fctx;
static size_t rd(void *c, char *b, size_t m) { return fread(b, 1, m, ((fctx *)c)->f); }
static cols_entry_t ent[60000]; static char pool[2000000];
int main(int argc, char **argv)
{
    int use_filter = !(argc > 1 && strcmp(argv[1], "tout") == 0);
    int mode = argc > 2 ? atoi(argv[2]) : 0;
    fctx cx = { fopen("sortie.gpx", "rb") }; static float X[20000], Y[20000], E[20000], W[20000];
    route_t r; route_init(&r, X, Y, E, 20000, 10.0f); gpx_load(rd, &cx, &r); fclose(cx.f);
    climb_cfg_t cfg; climb_cfg_default(&cfg); profile_smooth(E, r.n, cfg.smooth_half, W);
    static climb_t cl[64]; int k = climb_detect(E, r.n, W, &cfg, cl, 64);

    cols_db_t db; cols_init(&db, ent, 60000, pool, sizeof pool);
    if (use_filter) cols_filter_route(&db, &r, 3000.0f);
    fctx cf = { fopen("gps.csv", "rb") };
    clock_t t0 = clock(); int n = cols_load(rd, &cf, &db); double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC; fclose(cf.f);
    int nc = 0; for (int i = 0; i < db.n; i++) nc += db.e[i].kind == COLS_KIND_COL;
    printf("filtre %s : %d entrees gardees (%d cols), %d ecartees, memoire %u octets, lecture %.0f ms (PC)\n",
           use_filter ? "autour du parcours (3 km)" : "aucun", n, nc, db.skipped,
           (unsigned)(db.n * sizeof(cols_entry_t) + db.pool_used), ms);
    cols_cfg_t cc; cols_cfg_default(&cc); cc.place_mode = mode;
    int named = cols_name_climbs(&db, &cc, &r, E, cl, k);
    printf("%d montees nommees sur %d\n", named, k);
    for (int i = 0; i < k; i++) {
        double la, lo; route_unproject(&r, r.x[cl[i].end_idx], r.y[cl[i].end_idx], &la, &lo);
        float d = 0; int c = cols_match_summit(&db, &cc, la, lo, E[cl[i].end_idx], &d);
        printf("  %d. km %.1f a %.1f, %.1f km, %.1f %% : %s%s\n", i + 1, cl[i].start_m / 1000.0, cl[i].end_m / 1000.0,
               (double)(cl[i].length_m / 1000.0f), (double)cl[i].avg_grade_pct, cl[i].name[0] ? cl[i].name : "(sans nom)",
               c >= 0 ? "  [col a " : "");
        if (c >= 0) printf("%.0f m du sommet, alt %d m contre %.0f mesures]\n", (double)d, db.e[c].ele, (double)E[cl[i].end_idx]);
    }
    return 0;
}
