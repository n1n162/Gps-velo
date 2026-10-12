#include <stdio.h>
#include "climb.h"
#include "route.h"

typedef struct { FILE *f; } fctx;
static size_t rd(void *c, char *b, size_t m) { return fread(b, 1, m, ((fctx *)c)->f); }

/* Detecte les montees d'un GPX et affiche le tableau. Usage : test_climb sortie.gpx */
int main(int argc, char **argv)
{
    fctx cx = { fopen(argc > 1 ? argv[1] : "sortie.gpx", "rb") };
    if (!cx.f) { perror("gpx"); return 1; }
    static float X[20000], Y[20000], E[20000], W[20000];
    route_t r; route_init(&r, X, Y, E, 20000, 10.0f);
    if (gpx_load(rd, &cx, &r) < 0) { puts("GPX illisible"); return 1; }
    fclose(cx.f);
    climb_cfg_t cfg; climb_cfg_default(&cfg);
    profile_smooth(E, r.n, cfg.smooth_half, W);
    static climb_t cl[64];
    int k = climb_detect(E, r.n, W, &cfg, cl, 64);
    printf("%d points, %.1f km, %d montees\n", r.n, (double)(r.total_m / 1000.0f), k);
    printf("%-6s %-6s %7s %6s %6s %8s %s\n", "deb km", "fin km", "long m", "pente%", "D+ m", "score", "cat");
    for (int i = 0; i < k; i++)
        printf("%-6.1f %-6.1f %7.0f %6.1f %6.0f %8.0f %d\n", (double)cl[i].start_m / 1000.0, (double)cl[i].end_m / 1000.0,
               cl[i].length_m, cl[i].avg_grade_pct, cl[i].ascent_m, cl[i].score, climb_category(cl[i].score));
    return k > 0 ? 0 : 1;
}
