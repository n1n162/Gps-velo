#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "route.h"
#include "matcher.h"
#include "climb.h"

typedef struct { FILE *f; } fctx;
static size_t rd(void *c, char *buf, size_t max) { return fread(buf, 1, max, ((fctx *)c)->f); }

static unsigned rs = 12345u;
static float rnd(void) { rs = rs * 1664525u + 1013904223u; return (float)((rs >> 8) & 0xFFFF) / 32768.0f - 1.0f; }

static double LAT[20000], LON[20000], TRUE_D[20000];
static float SP[20000], HD[20000];
static int N;

typedef struct { int jump_at, jump_len, off_from, off_to; float off_m; } scen_t;

static int cmpf(const void *a, const void *b) { float x = *(const float *)a, y = *(const float *)b; return (x > y) - (x < y); }

/* Rejoue la sortie avec du bruit GPS (+-4 m, cap +-10 degres). matched[i] = distance trouvee. */
static void replay(const route_t *r, scen_t sc, float *matched, int *onr, int *glob)
{
    matcher_t m; matcher_init(&m, r);
    rs = 12345u;
    for (int i = 0; i < N; i++) {
        matched[i] = NAN; onr[i] = 0; glob[i] = 0;
        float rx = rnd(), ry = rnd(), rh = rnd();     /* tire toujours, pour garder la meme suite */
        if (i >= sc.jump_at && i < sc.jump_at + sc.jump_len) continue;
        double c = cos(LAT[i] * 3.14159265358979 / 180.0);
        double lat = LAT[i] + 4.0 * ry / 111320.0;
        double lon = LON[i] + 4.0 * rx / (111320.0 * c);
        if (i >= sc.off_from && i < sc.off_to) lon += sc.off_m / (111320.0 * c);
        float hd = HD[i] < 0 ? -1.0f : fmodf(HD[i] + 10.0f * rh + 360.0f, 360.0f);
        match_t mt;
        matcher_update(&m, lat, lon, SP[i], hd, &mt);
        matched[i] = mt.dist_m; onr[i] = mt.on_route; glob[i] = mt.global;
    }
}

int main(int argc, char **argv)
{
    const char *gpx = argc > 1 ? argv[1] : "sortie.gpx";
    fctx cx; cx.f = fopen(gpx, "rb");
    if (!cx.f) { perror(gpx); return 1; }
    static float X[20000], Y[20000], E[20000], work[20000];
    route_t r; route_init(&r, X, Y, E, 20000, 10.0f);
    int n = gpx_load(rd, &cx, &r);
    fclose(cx.f);
    printf("CHARGEUR : %d points GPX lus, %d points a pas de 10 m, longueur %.1f m, altitudes %s, tronque %d\n",
           r.raw_points, n, r.total_m, r.has_ele ? "oui" : "non", r.truncated);
    printf("memoire du parcours : %d octets (x, y, altitude)\n", n * 12);

    climb_cfg_t cfg; climb_cfg_default(&cfg);
    profile_smooth(E, n, cfg.smooth_half, work);
    static climb_t cl[64];
    int k = climb_detect(E, n, work, &cfg, cl, 64);
    printf("\nmontees %d\n", k);
    for (int i = 0; i < k; i++)
        printf("%-6.1f %-6.1f %7.0f %6.1f %6.0f %8.0f\n", (double)cl[i].start_m / 1000.0, (double)cl[i].end_m / 1000.0,
               cl[i].length_m, cl[i].avg_grade_pct, cl[i].ascent_m, cl[i].score);

    FILE *fp = fopen("replay.csv", "r");
    if (!fp) { perror("replay.csv"); return 1; }
    while (N < 20000 && fscanf(fp, "%lf,%lf,%lf,%f,%f", &LAT[N], &LON[N], &TRUE_D[N], &SP[N], &HD[N]) == 5) N++;
    fclose(fp);

    static float matched[20000]; static int onr[20000], glob[20000];
    static float errs[20000], errs_mv[20000];
    scen_t none = {-1, 0, -1, -1, 0.0f};
    replay(&r, none, matched, onr, glob);
    int ne = 0, nm = 0, off = 0, ng = 0; double sum = 0; float mx = 0;
    for (int i = 0; i < N; i++) {
        if (glob[i]) ng++;
        if (!onr[i]) { off++; continue; }
        float e = fabsf(matched[i] - (float)TRUE_D[i]);
        errs[ne++] = e; sum += e; if (e > mx) mx = e;
        if (SP[i] > 2.0f) errs_mv[nm++] = e;
    }
    qsort(errs, ne, sizeof(float), cmpf); qsort(errs_mv, nm, sizeof(float), cmpf);
    printf("\nSCENARIO A : rejeu complet (%d positions, bruit +-4 m)\n", N);
    printf("  hors parcours : %d, recherches globales : %d\n", off, ng);
    printf("  erreur de distance, toutes positions : moyenne %.1f m, 95 %% sous %.1f m, max %.1f m\n", sum / ne, errs[(int)(ne * 0.95)], mx);
    printf("  en mouvement (>2 m/s, %d positions) : 95 %% sous %.1f m, mediane %.1f m\n", nm, errs_mv[(int)(nm * 0.95)], errs_mv[nm / 2]);

    /* Demo montee : valeurs restantes avec la distance trouvee */
    printf("\nValeurs restantes avec la position trouvee (vraie distance -> trouvee)\n");
    float targets[] = {24000.f, 28000.f, 32000.f};
    for (int t = 0; t < 3; t++) {
        int i = 0; while (i < N - 1 && TRUE_D[i] < targets[t]) i++;
        int ci = climb_find(cl, k, matched[i]); climb_progress_t pr;
        if (ci >= 0 && climb_progress(&cl[ci], E, n, 10.0f, matched[i], &pr))
            printf("  vrai km %.2f -> trouve km %.2f : reste %.0f m, D+ restant %.0f m, pente moyenne restante %.1f %%\n",
                   TRUE_D[i] / 1000.0, matched[i] / 1000.0, pr.dist_remaining_m, pr.ascent_remaining_m, pr.avg_grade_remaining_pct);
    }

    /* B : perte du GPS, saut de 400 positions */
    scen_t sb = {5000, 400, -1, -1, 0.0f};
    replay(&r, sb, matched, onr, glob);
    printf("\nSCENARIO B : perte du signal, 400 positions sautees a partir de l'indice 5000 (%.1f km vrai a %.1f km)\n",
           TRUE_D[5000] / 1000.0, TRUE_D[5400] / 1000.0);
    for (int i = 5400; i < 5410; i++)
        printf("  apres le saut +%d : vrai %.0f m, trouve %.0f m, erreur %.0f m, sur parcours %d, recherche globale %d\n",
               i - 5400, TRUE_D[i], matched[i], matched[i] - TRUE_D[i], onr[i], glob[i]);

    /* C : sortie de parcours de 120 m pendant 200 positions */
    scen_t sc = {-1, 0, 8000, 8200, 120.0f};
    replay(&r, sc, matched, onr, glob);
    int offc = 0; for (int i = 8000; i < 8200; i++) if (!onr[i]) offc++;
    int rec = -1; for (int i = 8200; i < N; i++) if (onr[i] && fabsf(matched[i] - (float)TRUE_D[i]) < 15.0f) { rec = i; break; }
    printf("\nSCENARIO C : decalage de 120 m sur les positions 8000 a 8199\n");
    printf("  positions detectees hors parcours : %d sur 200\n", offc);
    printf("  retour sur la trace, erreur < 15 m : a l'indice %d (%d positions apres la fin du decalage)\n", rec, rec - 8200);
    return 0;
}
