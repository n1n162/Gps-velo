#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "matcher.h"
#include "cols.h"

typedef struct { FILE *f; } fctx;
static size_t rd(void *c, char *buf, size_t max) { return fread(buf, 1, max, ((fctx *)c)->f); }

static uint16_t fb[UI_H][UI_W];
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    const uint16_t *src = (const uint16_t *)px;
    int w = area->x2 - area->x1 + 1;
    for (int y = area->y1; y <= area->y2; y++)
        for (int x = area->x1; x <= area->x2; x++) fb[y][x] = src[(y - area->y1) * w + (x - area->x1)];
    lv_display_flush_ready(disp);
}

/* doigt simule */
static int ptr_x, ptr_y, ptr_down;
static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = ptr_x; data->point.y = ptr_y;
    data->state = ptr_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void step(int ms) { lv_tick_inc((uint32_t)ms); lv_timer_handler(); }
static void settle(void) { for (int i = 0; i < 14; i++) step(30); }
static void tap(int x, int y)
{
    ptr_x = x; ptr_y = y; ptr_down = 1; step(40); step(40);
    ptr_down = 0; step(40); step(40);
}
static void swipe(int x0, int x1, int y)
{
    ptr_x = x0; ptr_y = y; ptr_down = 1; step(40); step(40);
    int n = 6;
    for (int i = 1; i <= n; i++) { ptr_x = x0 + (x1 - x0) * i / n; step(40); }
    ptr_down = 0; step(40); step(40); settle();
}

static void save(const char *name)
{
    FILE *f = fopen(name, "wb"); fwrite(fb, sizeof(fb), 1, f); fclose(f);
}

/* actions declenchees par le toucher */
static int n_pause, n_lap, map_dir, open_n; static float zoom_w;
static void h_pause(void) { n_pause++; }
static void h_lap(void) { n_lap++; }
static void h_zoom(float w) { zoom_w = w; }
static void h_map(int d) { map_dir = d; }
static void h_open(int n) { open_n = n; }

typedef struct { unsigned t; double lat, lon, ele, dist, sp; int hr, cad; } row_t;
static row_t R[20000]; static int NR;

int main(void)
{
    fctx cx = { fopen("sortie.gpx", "rb") };
    static float X[20000], Y[20000], E[20000], W[20000];
    route_t r; route_init(&r, X, Y, E, 20000, 10.0f);
    gpx_load(rd, &cx, &r); fclose(cx.f);
    climb_cfg_t cfg; climb_cfg_default(&cfg);
    profile_smooth(E, r.n, cfg.smooth_half, W);
    static climb_t cl[64];
    int k = climb_detect(E, r.n, W, &cfg, cl, 64);
    static cols_entry_t ent[4000]; static char pool[100000]; cols_db_t db; cols_init(&db, ent, 4000, pool, sizeof pool);
    cols_filter_route(&db, &r, 3000.0f);
    { fctx cf = { fopen("gps.csv", "rb") }; if (cf.f) { cols_load(rd, &cf, &db); fclose(cf.f); } else puts("gps.csv absent : montees non nommees"); }
    { cols_cfg_t cc; cols_cfg_default(&cc); cols_name_climbs(&db, &cc, &r, E, cl, k); }
    FILE *fp = fopen("replay2.csv", "r"); int pw;
    while (NR < 20000 && fscanf(fp, "%u,%lf,%lf,%lf,%lf,%lf,%d,%d,%d", &R[NR].t, &R[NR].lat, &R[NR].lon, &R[NR].ele, &R[NR].dist, &R[NR].sp, &R[NR].hr, &R[NR].cad, &pw) == 9) NR++;
    fclose(fp);

    lv_init();
    lv_display_t *disp = lv_display_create(UI_W, UI_H);
    static uint16_t buf[UI_W * UI_H];
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_indev_t *ind = lv_indev_create();
    lv_indev_set_type(ind, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(ind, read_cb);
    lv_indev_set_display(ind, disp);
    ui_hooks = (ui_hooks_t){h_pause, h_lap, h_zoom, h_map, h_open};

    int i21 = 0, i29 = 0;
    while (i21 < NR - 1 && R[i21].dist < 21500.0) i21++;
    while (i29 < NR - 1 && R[i29].dist < 29800.0) i29++;

    /* ----- instant avant la montee (km 21,5) : pages 1 a 4 ----- */
    float pos = (float)R[i21].dist;
    float asc = 0, ref = (float)R[0].ele;
    for (int i = 0; i <= i21; i++) { float e = (float)R[i].ele; if (e - ref >= 1.0f) { asc += e - ref; ref = e; } else if (ref - e >= 1.0f) ref = e; }
    ui_data_t d; memset(&d, 0, sizeof(d));
    d.speed_kmh = (float)(R[i21].sp * 3.6); d.dist_km = (float)(R[i21].dist / 1000.0);
    d.time_s = R[i21].t - R[0].t; d.avg_kmh = d.dist_km / ((float)d.time_s / 3600.0f);
    d.hr = R[i21].hr; d.cad = R[i21].cad; d.ascent_m = asc;
    d.grade_pct = climb_grade_at(E, r.n, &cfg, pos);
    d.alt_m = E[(int)(pos / r.step_m)]; d.route_left_km = (r.total_m - pos) / 1000.0f;
    d.gps_ok = 1; d.battery_pct = 78; d.pages = 5;
    d.hour = (int)(((R[i21].t + 7200) / 3600) % 24); d.minute = (int)(((R[i21].t + 7200) / 60) % 60);
    int nci = climb_find(cl, k, pos);

    static climb_section_t esec[128];
    ui_elev_t ev; memset(&ev, 0, sizeof(ev));
    ev.ele = E; ev.n_ele = r.n; ev.step_m = r.step_m; ev.pos_m = pos; ev.window_m = 5000.0f;
    ev.n_sections = profile_sections(E, r.n, r.step_m, pos, pos + ev.window_m, 50.0f, esec, 128);
    ev.sections = esec; ev.climbs = cl; ev.n_climbs = k;
    ev.alt_m = d.alt_m; ev.grade_now_pct = d.grade_pct;
    ev.grade_ahead_pct = climb_grade_ahead(E, r.n, r.step_m, pos, 500.0f);
    { float a = 0, rf = d.alt_m; for (int i = (int)(pos / r.step_m); i < (int)((pos + ev.window_m) / r.step_m) && i < r.n; i++) { if (E[i] - rf >= 1.0f) { a += E[i] - rf; rf = E[i]; } else if (rf - E[i] >= 1.0f) rf = E[i]; } ev.ascent_ahead_m = a; }

    ui_nav_t nv; memset(&nv, 0, sizeof(nv));
    nv.route = &r; nv.pos_m = pos; nv.view_m = 1500.0f; nv.route_left_m = r.total_m - pos;
    if (nci >= 0 && cl[nci].start_m > pos) { nv.next_climb = &cl[nci]; nv.next_climb_in_m = cl[nci].start_m - pos; }

    static ui_list_row_t rows[6]; int nr = 0;
    for (int i = nci; i < k && nr < 6; i++, nr++) {
        rows[nr].number = i + 1; rows[nr].climb = &cl[i]; rows[nr].dist_to_start_m = cl[i].start_m - pos;
        rows[nr].in_progress = (pos >= cl[i].start_m && pos <= cl[i].end_m);
    }
    ui_list_t ls = { rows, nr, k, E, r.n, r.step_m };

    /* ----- page automatique : en pleine montee (km 29,8) ----- */
    ui_data_t dc = d;
    float posc = (float)R[i29].dist;
    dc.speed_kmh = (float)(R[i29].sp * 3.6); dc.hr = R[i29].hr; dc.cad = R[i29].cad;
    dc.grade_pct = climb_grade_at(E, r.n, &cfg, posc);
    int ci = climb_find(cl, k, posc);
    static climb_section_t sec[256];
    ui_climb_t c; memset(&c, 0, sizeof(c));
    c.index = ci; c.count = k; c.climb = &cl[ci]; c.n_sections = climb_sections(&cl[ci], E, r.n, &cfg, sec, 256); c.sections = sec;
    c.ele = E; c.n_ele = r.n; c.step_m = r.step_m; c.pos_m = posc;
    climb_progress(&cl[ci], E, r.n, r.step_m, posc, &c.progress);
    c.grade_now_pct = dc.grade_pct; c.grade_ahead_pct = climb_grade_ahead(E, r.n, r.step_m, posc, 500.0f);
    if (ci + 1 < k) { c.next = &cl[ci + 1]; c.next_number = ci + 2; c.next_in_m = cl[ci + 1].start_m - posc; }

    static ui_elev_t strip; strip = ev; strip.window_m = 5000.0f;
    ui_pager_t pg; ui_pager_init(&pg);
    const char *names[5] = {"donnees", "nav", "elev", "list", "climb"};
    ui_data_t dd[5];
    for (int i = 0; i < 5; i++) { dd[i] = (i == 4) ? dc : d; dd[i].page = i; }
    ui_pager_add(&pg, ui_data_screen(&dd[0], &strip));
    ui_pager_add(&pg, ui_nav_screen(&nv, &dd[1]));
    ui_pager_add(&pg, ui_elev_screen(&ev, &dd[2]));
    ui_pager_add(&pg, ui_list_screen(&ls, &dd[3]));
    ui_pager_add(&pg, ui_climb_screen(&c, &dd[4]));

    for (int i = 0; i < 5; i++) {      /* captures de chaque page */
        lv_screen_load(pg.pages[i]); pg.cur = i; settle();
        char f[64]; snprintf(f, sizeof f, "screen_%s.rgb565", names[i]); save(f);
    }

    /* ----- test du toucher ----- */
    lv_screen_load(pg.pages[0]); pg.cur = 0; settle();
    printf("TEST TOUCHER\n");
    swipe(40, 280, 200);  printf("  glisser a droite sur la premiere page : page %d (attendu 0)\n", pg.cur);
    swipe(280, 40, 200);  printf("  glisser a gauche : page %d (attendu 1, trace)\n", pg.cur);
    tap(289, 59);         printf("  toucher le bouton + de la carte : map_zoom(%d) (attendu 1)\n", map_dir);
    tap(289, 113);        printf("  toucher le bouton - de la carte : map_zoom(%d) (attendu -1)\n", map_dir);
    swipe(280, 40, 200);  printf("  glisser a gauche : page %d (attendu 2, altitude)\n", pg.cur);
    save("screen_after_swipes.rgb565");
    tap(285, 50);         printf("  toucher la pastille 10 km : elev_zoom(%.0f) (attendu 10000)\n", (double)zoom_w);
    tap(171, 50);         printf("  toucher la pastille 2 km : elev_zoom(%.0f) (attendu 2000)\n", (double)zoom_w);
    swipe(280, 40, 200);  printf("  glisser a gauche : page %d (attendu 3, liste)\n", pg.cur);
    tap(160, 64 + 69 + 30); printf("  toucher la 2e montee de la liste : open_climb(%d) (attendu %d)\n", open_n, rows[1].number);
    swipe(40, 280, 200); swipe(40, 280, 200); swipe(40, 280, 200);
    printf("  glisser a droite 3 fois : page %d (attendu 0)\n", pg.cur);
    tap(82, 447);         printf("  toucher PAUSE : %d appel (attendu 1)\n", n_pause);
    tap(238, 447);        printf("  toucher TOUR : %d appel (attendu 1)\n", n_lap);
    tap(160, 150);        printf("  toucher une zone vide : pause %d, tour %d (inchanges)\n", n_pause, n_lap);
    return 0;
}
