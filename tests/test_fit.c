#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fit.h"

typedef struct { FILE *f; } fx;
static int w_write(void *c, const void *b, size_t n) { return fwrite(b, 1, n, ((fx *)c)->f) == n ? 0 : -1; }
static int w_write_at(void *c, uint32_t off, const void *b, size_t n)
{
    FILE *f = ((fx *)c)->f; long cur = ftell(f);
    fseek(f, (long)off, SEEK_SET); size_t r = fwrite(b, 1, n, f); fseek(f, cur, SEEK_SET);
    return r == n ? 0 : -1;
}
static int w_flush(void *c) { return fflush(((fx *)c)->f); }
static int r_read(void *c, uint32_t off, void *b, size_t n)
{
    FILE *f = ((fx *)c)->f; fseek(f, (long)off, SEEK_SET); return fread(b, 1, n, f) == n ? 0 : -1;
}

static fit_sample_t S[20000];
static int N;

static void load(void)
{
    FILE *fp = fopen("replay2.csv", "r");
    unsigned t; double la, lo, el, d, sp; int hr, cad, pw;
    while (N < 20000 && fscanf(fp, "%u,%lf,%lf,%lf,%lf,%lf,%d,%d,%d", &t, &la, &lo, &el, &d, &sp, &hr, &cad, &pw) == 9) {
        fit_sample_t *s = &S[N++];
        s->time_unix = t; s->has_fix = 1; s->lat_deg = la; s->lon_deg = lo;
        s->has_alt = 1; s->alt_m = (float)el; s->dist_m = (float)d; s->speed_mps = (float)sp;
        s->hr = hr; s->cad = cad; s->power = pw;
    }
    fclose(fp);
}

int main(void)
{
    load();
    printf("%d echantillons charges\n", N);

    /* Scenario 1 : sortie complete, avec une pause de 100 s et un tour a 50 km */
    fx out = { fopen("ride.fit", "wb+") };
    fit_io_t io = { w_write, w_write_at, w_flush, &out };
    static fit_writer_t w;
    if (fit_begin(&w, &io, S[0].time_unix, 7200)) { puts("erreur fit_begin"); return 1; }
    int lapped = 0, written = 0;
    for (int i = 0; i < N; i++) {
        if (i > 10000 && i < 10100) continue;                 /* pause : aucun enregistrement */
        if (i == 10100) fit_resume(&w, S[i].time_unix);
        if (!lapped && S[i].dist_m >= 50000.0f) { fit_lap(&w, S[i].time_unix); lapped = 1; }
        fit_add_sample(&w, &S[i]);
        written++;
        if (i == 10000) fit_pause(&w, S[i].time_unix);
        if (i % 30 == 29) fit_flush(&w);
    }
    int r = fit_end(&w);
    long sz = ftell(out.f);
    fclose(out.f);
    printf("ride.fit : fit_end=%d, %d enregistrements, %ld octets (%.1f octets par seconde)\n", r, written, sz, (double)sz / N);

    /* Scenario 2 : coupure brusque apres 7000 echantillons, derniere ecriture tronquee */
    fx cr = { fopen("ride_crash.fit", "wb+") };
    fit_io_t io2 = { w_write, w_write_at, w_flush, &cr };
    static fit_writer_t w2;
    fit_begin(&w2, &io2, S[0].time_unix, 7200);
    for (int i = 0; i < 7000; i++) { fit_add_sample(&w2, &S[i]); if (i % 30 == 29) fit_flush(&w2); }
    fclose(cr.f);                                              /* pas de fit_end : le tampon interne est perdu */
    FILE *t = fopen("ride_crash.fit", "rb"); fseek(t, 0, SEEK_END); long csz = ftell(t); fclose(t);
    if (truncate("ride_crash.fit", csz - 7) != 0) puts("troncature impossible");
    printf("ride_crash.fit : %ld octets, tronque de 7 octets pour simuler une ecriture coupee\n", csz - 7);

    fx src = { fopen("ride_crash.fit", "rb") };
    fseek(src.f, 0, SEEK_END); uint32_t ssz = (uint32_t)ftell(src.f);
    fit_src_t fs = { r_read, &src, ssz };
    fx dst = { fopen("ride_recovered.fit", "wb+") };
    fit_io_t io3 = { w_write, w_write_at, w_flush, &dst };
    static fit_writer_t w3;
    int n = fit_recover(&fs, &io3, 7200, &w3);
    fclose(src.f); fclose(dst.f);
    printf("recuperation : %d points recuperes sur 7000 ecrits\n", n);
    return 0;
}
