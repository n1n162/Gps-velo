#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

ui_hooks_t ui_hooks;

#define COL_BG    lv_color_hex(0x0E1217)
#define COL_PANEL lv_color_hex(0x171D25)
#define COL_LINE  lv_color_hex(0x252D38)
#define COL_TEXT  lv_color_hex(0xF2F5F8)
#define COL_DIM   lv_color_hex(0x8A94A3)
#define COL_ACC   lv_color_hex(0x4AA3FF)
#define COL_OK    lv_color_hex(0x3FB950)
#define COL_BAD   lv_color_hex(0xE5484D)
#define COL_ORG   lv_color_hex(0xF0883E)

#define FONT_S  (&ui_font_14)
#define FONT_N  (&ui_font_16)
#define FONT_M  (&ui_font_20)
#define FONT_L  (&ui_font_28)
#define FONT_XL (&ui_font_48)

/* ---------- couleurs ---------- */
lv_color_t ui_level_color(int level)
{
    static const uint32_t c[6] = {0x7A8290, 0x3FB950, 0xE3C33B, 0xF0883E, 0xE5484D, 0xA51D2D};
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    return lv_color_hex(c[level]);
}

lv_color_t ui_category_color(int cat)
{
    /* table des categories Garmin : HC rouge fonce, 1 rouge, 2 orange, 3 jaune, 4 et non categorisee vert */
    static const uint32_t c[6] = {0xA51D2D, 0xE5484D, 0xF0883E, 0xE3C33B, 0x3FB950, 0x2E8B45};
    if (cat < 0 || cat > 5) cat = 5;
    return lv_color_hex(c[cat]);
}

static const char *category_name(int cat)
{
    static const char *n[6] = {"HC", "1", "2", "3", "4", "-"};
    return (cat < 0 || cat > 5) ? "-" : n[cat];
}

static int level_of(float g)
{
    if (g < 1.0f) return 0;
    if (g < 3.0f) return 1;
    if (g < 6.0f) return 2;
    if (g < 9.0f) return 3;
    if (g < 12.0f) return 4;
    return 5;
}

/* ---------- briques ---------- */
static lv_obj_t *box(lv_obj_t *p, int x, int y, int w, int h, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    return o;
}

static lv_obj_t *lbl(lv_obj_t *p, int x, int y, const lv_font_t *f, lv_color_t c, const char *t)
{
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_obj_set_pos(l, x, y);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

static lv_obj_t *lbl_c(lv_obj_t *p, int x, int y, int w, const lv_font_t *f, lv_color_t c, const char *t)
{
    lv_obj_t *l = lbl(p, x, y, f, c, t);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    return l;
}

static lv_obj_t *lbl_dot(lv_obj_t *p, int x, int y, int w, int h, const lv_font_t *f, lv_color_t c, const char *t)
{
    lv_obj_t *l = lbl(p, x, y, f, c, t);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_size(l, w, h);
    return l;
}

static lv_obj_t *lbl_r(lv_obj_t *p, int right_margin, int y, const lv_font_t *f, lv_color_t c, const char *t)
{
    lv_obj_t *l = lbl(p, 0, y, f, c, t);
    lv_obj_align(l, LV_ALIGN_TOP_RIGHT, -right_margin, y);
    return l;
}

static lv_obj_t *new_screen(void)
{
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s, COL_BG, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    return s;
}

static lv_obj_t *button(lv_obj_t *p, int x, int y, int w, int h, lv_color_t bg, lv_color_t fg, const char *txt,
                        const lv_font_t *font, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *b = box(p, x, y, w, h, bg);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lbl_c(b, 0, (h - (font == FONT_S ? 16 : 24)) / 2, w, font, fg, txt);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    return b;
}

static void status_bar(lv_obj_t *s, const ui_data_t *d)
{
    char b[32];
    box(s, 0, 0, UI_W, 28, COL_PANEL);
    lbl(s, 8, 6, FONT_S, d->gps_ok ? COL_OK : COL_BAD, LV_SYMBOL_GPS);
    snprintf(b, sizeof b, LV_SYMBOL_BLUETOOTH " %s", d->hr >= 0 ? "FC" : "--");
    lbl(s, 30, 6, FONT_S, d->hr >= 0 ? COL_OK : COL_DIM, b);
    snprintf(b, sizeof b, "%02d:%02d", d->hour, d->minute);
    lbl_c(s, 90, 6, 60, FONT_S, COL_TEXT, b);
    for (int i = 0; i < d->pages; i++) {
        lv_obj_t *dot = box(s, 160 + i * 11, 11, 6, 6, i == d->page ? COL_TEXT : lv_color_hex(0x4B5563));
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    }
    const char *sym = d->battery_pct > 75 ? LV_SYMBOL_BATTERY_FULL : d->battery_pct > 50 ? LV_SYMBOL_BATTERY_3 :
                      d->battery_pct > 25 ? LV_SYMBOL_BATTERY_2 : d->battery_pct > 10 ? LV_SYMBOL_BATTERY_1 : LV_SYMBOL_BATTERY_EMPTY;
    snprintf(b, sizeof b, "%s %d%%", sym, d->battery_pct);
    lbl_r(s, 8, 6, FONT_S, d->battery_pct > 15 ? COL_DIM : COL_BAD, b);
}

static void cell(lv_obj_t *s, int x, int y, const char *title, const char *value, lv_color_t c)
{
    lbl(s, x, y, FONT_S, COL_DIM, title);
    lbl(s, x, y + 17, FONT_L, c, value);
}

/* ---------- profil colore des prochains kilometres ---------- */
static void draw_profile(lv_obj_t *s, int x, int y, int w, int h, const ui_elev_t *e, int with_flags)
{
    char b[32];
    float emin = 1e9f, emax = -1e9f;
    for (int i = 0; i < e->n_sections; i++) {
        int ei = (int)(e->sections[i].end_m / e->step_m);
        if (ei >= e->n_ele) ei = e->n_ele - 1;
        if (e->ele[ei] < emin) emin = e->ele[ei];
        if (e->ele[ei] > emax) emax = e->ele[ei];
    }
    float range = emax - emin;
    if (range < 40.0f) range = 40.0f;
    for (int i = 0; i < e->n_sections; i++) {
        const climb_section_t *sc = &e->sections[i];
        int x0 = x + (i * w) / e->n_sections;
        int x1 = x + ((i + 1) * w) / e->n_sections;
        int ei = (int)(sc->end_m / e->step_m);
        if (ei >= e->n_ele) ei = e->n_ele - 1;
        int bh = 6 + (int)((e->ele[ei] - emin) / range * (float)(h - 8));
        box(s, x0, y + h - bh, x1 - x0, bh, ui_level_color(sc->level));
    }
    if (with_flags) {
        for (int i = 0; i < e->n_climbs; i++) {
            const climb_t *c = &e->climbs[i];
            float rel = c->start_m - e->pos_m;
            if (rel <= 0.0f || rel > e->window_m) continue;
            int fx = x + (int)(rel / e->window_m * (float)w);
            box(s, fx - 1, y, 2, h, COL_TEXT);
            lv_obj_t *flag = box(s, fx, y, 58, 18, ui_category_color(climb_category(c->score)));
            lv_obj_set_style_radius(flag, 4, 0);
            snprintf(b, sizeof b, "%.1f km", (double)(rel / 1000.0f));
            lbl_c(s, fx, y + 2, 58, FONT_S, COL_BG, b);
        }
    }
    box(s, x, y, 2, h, COL_TEXT);
    lv_obj_t *dot = box(s, x - 5, y + h - 22, 12, 12, COL_TEXT);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 3, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0xFF8A1F), 0);
}

/* ---------- actions tactiles ---------- */
static void cb_pause(lv_event_t *e) { (void)e; if (ui_hooks.pause) ui_hooks.pause(); }
static void cb_lap(lv_event_t *e) { (void)e; if (ui_hooks.lap) ui_hooks.lap(); }
static void cb_zoom(lv_event_t *e) { if (ui_hooks.elev_zoom) ui_hooks.elev_zoom((float)(intptr_t)lv_event_get_user_data(e)); }
static void cb_map_zoom(lv_event_t *e) { if (ui_hooks.map_zoom) ui_hooks.map_zoom((int)(intptr_t)lv_event_get_user_data(e)); }
static void cb_climb(lv_event_t *e) { if (ui_hooks.open_climb) ui_hooks.open_climb((int)(intptr_t)lv_event_get_user_data(e)); }

/* ---------- page 1 : donnees ---------- */
lv_obj_t *ui_data_screen(const ui_data_t *d, const ui_elev_t *strip)
{
    lv_obj_t *s = new_screen();
    char b[48];
    status_bar(s, d);

    snprintf(b, sizeof b, "%.1f", (double)d->speed_kmh);
    lbl(s, 8, 34, FONT_XL, COL_TEXT, b);
    lbl(s, 136, 74, FONT_M, COL_DIM, "km/h");
    lv_obj_t *chip = box(s, 214, 36, 98, 68, ui_level_color(level_of(d->grade_pct)));
    lv_obj_set_style_radius(chip, 10, 0);
    lbl_c(s, 214, 41, 98, FONT_S, COL_TEXT, "PENTE");
    snprintf(b, sizeof b, "%.1f %%", (double)d->grade_pct);
    lbl_c(s, 214, 62, 98, FONT_L, COL_TEXT, b);

    int ys[4] = {112, 166, 220, 274};
    for (int i = 0; i < 4; i++) box(s, 8, ys[i] - 2, UI_W - 16, 1, COL_LINE);
    snprintf(b, sizeof b, "%u:%02u:%02u", d->time_s / 3600, (d->time_s / 60) % 60, d->time_s % 60);
    cell(s, 10, ys[0], "TEMPS", b, COL_TEXT);
    snprintf(b, sizeof b, "%.1f km", (double)d->dist_km);
    cell(s, 170, ys[0], "DISTANCE", b, COL_TEXT);
    if (d->hr >= 0) snprintf(b, sizeof b, "%d bpm", d->hr); else snprintf(b, sizeof b, "--");
    cell(s, 10, ys[1], "FRÉQ. CARDIAQUE", b, d->hr >= 0 ? COL_BAD : COL_DIM);
    if (d->cad >= 0) snprintf(b, sizeof b, "%d rpm", d->cad); else snprintf(b, sizeof b, "--");
    cell(s, 170, ys[1], "CADENCE", b, d->cad >= 0 ? COL_ACC : COL_DIM);
    snprintf(b, sizeof b, "%.0f m", (double)d->ascent_m);
    cell(s, 10, ys[2], "DÉNIVELÉ +", b, COL_TEXT);
    snprintf(b, sizeof b, "%.1f km/h", (double)d->avg_kmh);
    cell(s, 170, ys[2], "VITESSE MOY.", b, COL_TEXT);
    snprintf(b, sizeof b, "%.0f m", (double)d->alt_m);
    cell(s, 10, ys[3], "ALTITUDE", b, COL_TEXT);
    snprintf(b, sizeof b, "%.1f km", (double)d->route_left_km);
    cell(s, 170, ys[3], "RESTE", b, COL_TEXT);

    box(s, 8, 328, UI_W - 16, 1, COL_LINE);
    lbl(s, 10, 334, FONT_S, COL_DIM, "PROCHAINS KM");
    if (strip) draw_profile(s, 8, 352, UI_W - 16, 58, strip, 0);

    button(s, 8, 422, 148, 50, d->paused ? COL_OK : lv_color_hex(0x3A2A1A), d->paused ? COL_BG : COL_ORG,
           d->paused ? "REPRENDRE" : "PAUSE", FONT_M, cb_pause, NULL);
    button(s, 164, 422, 148, 50, lv_color_hex(0x16283A), COL_ACC, "TOUR", FONT_M, cb_lap, NULL);
    return s;
}

/* ---------- page 2 : trace ---------- */
#define MAP_Y 28
#define MAP_H 380
#define PTS_MAX 520
static lv_point_precise_t pts_done[PTS_MAX], pts_left[PTS_MAX], pts_climb[PTS_MAX];

static void polyline(lv_obj_t *map, lv_point_precise_t *p, int n, int width, lv_color_t col)
{
    if (n < 2) return;
    lv_obj_t *ln = lv_line_create(map);
    lv_line_set_points(ln, p, (uint32_t)n);
    lv_obj_set_style_line_width(ln, width, 0);
    lv_obj_set_style_line_color(ln, col, 0);
    lv_obj_set_style_line_rounded(ln, true, 0);
    lv_obj_set_pos(ln, 0, 0);
    lv_obj_set_size(ln, UI_W, MAP_H);
    lv_obj_remove_flag(ln, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t *ui_nav_screen(const ui_nav_t *n, const ui_data_t *d)
{
    lv_obj_t *s = new_screen();
    char b[48];
    status_bar(s, d);
    lv_obj_t *map = box(s, 0, MAP_Y, UI_W, MAP_H, lv_color_hex(0x141A21));
    const route_t *r = n->route;
    float step = r->step_m;
    int ci = (int)(n->pos_m / step);
    if (ci < 0) ci = 0;
    if (ci >= r->n) ci = r->n - 1;
    float cx = r->x[ci], cy = r->y[ci];
    float scale = (UI_W / 2.0f) / n->view_m;
    int m = (int)(n->view_m * 3.0f / step) + 20;
    int lo = ci - m, hi = ci + m;
    if (lo < 0) lo = 0;
    if (hi > r->n - 1) hi = r->n - 1;
    int sub = 1;
    while ((ci - lo) / sub >= PTS_MAX - 1 || (hi - ci) / sub >= PTS_MAX - 1) sub++;
    int nd = 0, nl = 0, nc = 0;
    for (int i = lo; i <= ci; i += sub, nd++) {
        pts_done[nd].x = (lv_value_precise_t)(UI_W / 2 + (r->x[i] - cx) * scale);
        pts_done[nd].y = (lv_value_precise_t)(MAP_H / 2 - (r->y[i] - cy) * scale);
    }
    for (int i = ci; i <= hi; i += sub, nl++) {
        pts_left[nl].x = (lv_value_precise_t)(UI_W / 2 + (r->x[i] - cx) * scale);
        pts_left[nl].y = (lv_value_precise_t)(MAP_H / 2 - (r->y[i] - cy) * scale);
    }
    if (n->next_climb) {
        int a = n->next_climb->start_idx, e = n->next_climb->end_idx;
        if (a < lo) a = lo;
        if (e > hi) e = hi;
        for (int i = a; i <= e && nc < PTS_MAX; i += sub, nc++) {
            pts_climb[nc].x = (lv_value_precise_t)(UI_W / 2 + (r->x[i] - cx) * scale);
            pts_climb[nc].y = (lv_value_precise_t)(MAP_H / 2 - (r->y[i] - cy) * scale);
        }
    }
    polyline(map, pts_done, nd, 6, lv_color_hex(0x4B5563));
    polyline(map, pts_left, nl, 6, COL_ACC);
    polyline(map, pts_climb, nc, 6, COL_ORG);

    lv_obj_t *dot = box(map, UI_W / 2 - 9, MAP_H / 2 - 9, 18, 18, COL_TEXT);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 3, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0xFF8A1F), 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *nb = box(map, 8, 8, 26, 26, COL_PANEL);
    lv_obj_set_style_radius(nb, LV_RADIUS_CIRCLE, 0);
    lbl_c(map, 8, 12, 26, FONT_S, COL_TEXT, "N");
    lv_obj_t *zin = button(map, UI_W - 54, 8, 46, 46, COL_PANEL, COL_TEXT, "+", FONT_M, cb_map_zoom, (void *)(intptr_t)1);
    lv_obj_set_style_radius(zin, LV_RADIUS_CIRCLE, 0);
    lv_obj_t *zout = button(map, UI_W - 54, 62, 46, 46, COL_PANEL, COL_TEXT, "-", FONT_M, cb_map_zoom, (void *)(intptr_t)-1);
    lv_obj_set_style_radius(zout, LV_RADIUS_CIRCLE, 0);
    int bar = (int)(1000.0f * scale);
    box(map, 10, MAP_H - 18, bar, 3, COL_TEXT);
    lbl(map, 10 + bar + 6, MAP_H - 26, FONT_S, COL_DIM, "1 km");

    box(s, 0, MAP_Y + MAP_H, UI_W, UI_H - MAP_Y - MAP_H, COL_PANEL);
    lbl(s, 12, 414, FONT_S, COL_DIM, "RESTE");
    snprintf(b, sizeof b, "%.1f km", (double)(n->route_left_m / 1000.0f));
    lbl(s, 12, 432, FONT_L, COL_TEXT, b);
    if (n->next_climb) {
        snprintf(b, sizeof b, "MONTÉE DANS %.1f km", (double)(n->next_climb_in_m / 1000.0f));
        lbl_r(s, 12, 412, FONT_S, COL_ORG, b);
        if (n->next_climb->name[0]) {
            lv_obj_t *nm = lbl_dot(s, 142, 430, 170, 20, FONT_N, COL_TEXT, n->next_climb->name);
            lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_RIGHT, 0);
        }
        snprintf(b, sizeof b, "%.1f km  %.1f %%", (double)(n->next_climb->length_m / 1000.0f), (double)n->next_climb->avg_grade_pct);
        lbl_r(s, 12, 452, FONT_S, COL_DIM, b);
    }
    return s;
}

/* ---------- page 3 : altitude ---------- */
lv_obj_t *ui_elev_screen(const ui_elev_t *e, const ui_data_t *d)
{
    lv_obj_t *s = new_screen();
    char b[64];
    status_bar(s, d);
    lbl(s, 10, 38, FONT_M, COL_TEXT, "ALTITUDE");
    static const float zooms[3] = {2000.0f, 5000.0f, 10000.0f};
    static const char *names[3] = {"2 km", "5 km", "10 km"};
    for (int i = 0; i < 3; i++) {
        int active = fabsf(e->window_m - zooms[i]) < 1.0f;
        button(s, 140 + i * 59, 34, 54, 32, active ? COL_ACC : COL_PANEL, active ? COL_BG : COL_DIM, names[i], FONT_S, cb_zoom, (void *)(intptr_t)zooms[i]);
    }
    draw_profile(s, 8, 76, UI_W - 16, 230, e, 1);
    float tick = e->window_m <= 2500.0f ? 500.0f : (e->window_m <= 6000.0f ? 1000.0f : 2000.0f);
    for (float t = 0.0f; t <= e->window_m + 1.0f; t += tick) {
        int x = 8 + (int)(t / e->window_m * (UI_W - 16));
        int lx = x - 20;
        if (lx < 0) lx = 0;
        if (lx > UI_W - 40) lx = UI_W - 40;
        snprintf(b, sizeof b, "%g", (double)(t / 1000.0f));
        lbl_c(s, lx, 310, 40, FONT_S, COL_DIM, b);
    }
    box(s, 0, 336, UI_W, UI_H - 336, COL_PANEL);
    snprintf(b, sizeof b, "%.0f m", (double)e->alt_m);
    lbl_c(s, 0, 346, 106, FONT_L, COL_TEXT, b);
    lbl_c(s, 0, 380, 106, FONT_S, COL_DIM, "ALTITUDE");
    box(s, 106, 350, 1, 50, COL_LINE);
    snprintf(b, sizeof b, "%.1f %%", (double)e->grade_now_pct);
    lbl_c(s, 107, 346, 106, FONT_L, ui_level_color(level_of(e->grade_now_pct)), b);
    lbl_c(s, 107, 380, 106, FONT_S, COL_DIM, "PENTE");
    box(s, 213, 350, 1, 50, COL_LINE);
    snprintf(b, sizeof b, "+%.0f m", (double)e->ascent_ahead_m);
    lbl_c(s, 214, 346, 106, FONT_L, COL_TEXT, b);
    lbl_c(s, 214, 380, 106, FONT_S, COL_DIM, "D+ À VENIR");
    box(s, 8, 414, UI_W - 16, 1, COL_LINE);
    snprintf(b, sizeof b, "Pente des 500 m devant : %.1f %%", (double)e->grade_ahead_pct);
    lbl_c(s, 0, 430, UI_W, FONT_M, ui_level_color(level_of(e->grade_ahead_pct)), b);
    return s;
}

/* ---------- page 4 : liste des montees ---------- */
lv_obj_t *ui_list_screen(const ui_list_t *l, const ui_data_t *d)
{
    lv_obj_t *s = new_screen();
    char b[64];
    status_bar(s, d);
    lbl(s, 10, 36, FONT_M, COL_TEXT, "MONTÉES");
    snprintf(b, sizeof b, "%d sur le parcours", l->total);
    lbl_r(s, 10, 41, FONT_S, COL_DIM, b);
    for (int i = 0; i < l->n_rows && i < 6; i++) {
        const ui_list_row_t *r = &l->rows[i];
        int y = 64 + i * 69;
        int cat = climb_category(r->climb->score);
        lv_obj_t *row = box(s, 0, y, UI_W, 68, COL_BG);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, cb_climb, LV_EVENT_CLICKED, (void *)(intptr_t)r->number);
        box(s, 8, y + 67, UI_W - 16, 1, COL_LINE);
        lv_obj_t *chip = box(s, 8, y + 10, 46, 46, ui_category_color(cat));
        lv_obj_set_style_radius(chip, 8, 0);
        lv_obj_remove_flag(chip, LV_OBJ_FLAG_CLICKABLE);
        lbl_c(s, 8, y + 19, 46, FONT_M, cat == 3 ? COL_BG : COL_TEXT, category_name(cat));
        char title[48];
        if (r->climb->name[0]) snprintf(title, sizeof title, "%s", r->climb->name);
        else snprintf(title, sizeof title, "Montée %d", r->number);
        lbl_dot(s, 64, y + 6, 184, 20, FONT_N, COL_TEXT, title);
        if (r->in_progress) snprintf(b, sizeof b, "EN COURS");
        else snprintf(b, sizeof b, "Dans %.1f km", (double)(r->dist_to_start_m / 1000.0f));
        lbl(s, 64, y + 27, FONT_S, r->in_progress ? COL_ORG : COL_DIM, b);
        snprintf(b, sizeof b, "%.1f km  %.1f %%  +%.0f m", (double)(r->climb->length_m / 1000.0f),
                 (double)r->climb->avg_grade_pct, (double)r->climb->ascent_m);
        lbl(s, 64, y + 45, FONT_S, COL_DIM, b);
        climb_section_t sec[24];
        int ns = profile_sections(l->ele, l->n_ele, l->step_m, r->climb->start_m, r->climb->end_m,
                                  r->climb->length_m / 20.0f, sec, 24);
        float emin = l->ele[r->climb->start_idx], emax = l->ele[r->climb->end_idx];
        for (int k = 0; k < ns; k++) {
            int x0 = 254 + (k * 58) / ns, x1 = 254 + ((k + 1) * 58) / ns;
            int ei = (int)(sec[k].end_m / l->step_m);
            if (ei >= l->n_ele) ei = l->n_ele - 1;
            int h = 4 + (int)((l->ele[ei] - emin) / (emax - emin + 0.1f) * 36.0f);
            lv_obj_t *bar = box(s, x0, y + 56 - h, x1 - x0, h, ui_level_color(sec[k].level));
            lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    return s;
}

/* ---------- page auto : montee en cours (Garmin ClimbPro) ---------- */
#define PX 8
#define PW (UI_W - 16)
#define PY 106
#define PH 152

lv_obj_t *ui_climb_screen(const ui_climb_t *c, const ui_data_t *d)
{
    lv_obj_t *s = new_screen();
    char b[96];
    status_bar(s, d);
    const climb_t *cl = c->climb;
    int cat = climb_category(cl->score);

    if (cl->name[0]) snprintf(b, sizeof b, "%s", cl->name);
    else snprintf(b, sizeof b, "MONTÉE %d/%d", c->index + 1, c->count);
    lbl_dot(s, 10, 36, 196, 26, strlen(b) > 15 ? FONT_N : FONT_M, COL_TEXT, b);
    if (strlen(b) > 15) lv_obj_set_y(lv_obj_get_child(s, -1), 40);
    lv_obj_t *chip = box(s, 212, 32, 100, 34, ui_level_color(level_of(c->grade_now_pct)));
    lv_obj_set_style_radius(chip, 9, 0);
    snprintf(b, sizeof b, "%.1f %%", (double)c->grade_now_pct);
    lbl_c(s, 212, 37, 100, FONT_M, COL_TEXT, b);

    lv_obj_t *badge = box(s, 10, 64, 56, 20, ui_category_color(cat));
    lv_obj_set_style_radius(badge, 6, 0);
    snprintf(b, sizeof b, "CAT %s", category_name(cat));
    lbl_c(s, 10, 66, 56, FONT_S, cat == 3 ? COL_BG : COL_TEXT, b);
    snprintf(b, sizeof b, "MONTÉE %d/%d", c->index + 1, c->count);
    lbl(s, 74, 66, FONT_S, COL_DIM, b);
    snprintf(b, sizeof b, "500 m: %.1f %%", (double)c->grade_ahead_pct);
    lbl_r(s, 10, 66, FONT_S, ui_level_color(level_of(c->grade_ahead_pct)), b);
    snprintf(b, sizeof b, "%.1f km  %.1f %%  +%.0f m", (double)(cl->length_m / 1000.0f), (double)cl->avg_grade_pct, (double)cl->ascent_m);
    lbl(s, 10, 88, FONT_S, COL_DIM, b);

    float emin = 1e9f, emax = -1e9f;
    for (int i = cl->start_idx; i <= cl->end_idx && i < c->n_ele; i++) {
        if (c->ele[i] < emin) emin = c->ele[i];
        if (c->ele[i] > emax) emax = c->ele[i];
    }
    float frac = (c->pos_m - cl->start_m) / (cl->end_m - cl->start_m);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int xpos = PX + (int)(frac * PW);
    int top_at_pos = PY + PH;
    for (int i = 0; i < c->n_sections; i++) {
        const climb_section_t *sc = &c->sections[i];
        int x0 = PX + (i * PW) / c->n_sections;
        int x1 = PX + ((i + 1) * PW) / c->n_sections;
        int ei = (int)(sc->end_m / c->step_m);
        if (ei >= c->n_ele) ei = c->n_ele - 1;
        float t = (c->ele[ei] - emin) / (emax - emin + 0.1f);
        int h = 8 + (int)(t * (PH - 10));
        lv_obj_t *bar = box(s, x0, PY + PH - h, x1 - x0, h, ui_level_color(sc->level));
        if (sc->end_m <= c->pos_m) lv_obj_set_style_bg_opa(bar, 90, 0);
        if (xpos >= x0 && xpos <= x1) top_at_pos = PY + PH - h;
    }
    box(s, xpos - 1, PY - 2, 2, PH + 2, COL_TEXT);
    lv_obj_t *dot = box(s, xpos - 7, top_at_pos - 7, 14, 14, COL_TEXT);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 3, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0xFF8A1F), 0);
    snprintf(b, sizeof b, "%.0f m", (double)emax);
    lbl_r(s, 10, PY - 17, FONT_S, COL_DIM, b);
    snprintf(b, sizeof b, "%.0f m", (double)emin);
    lbl(s, 10, PY + PH + 2, FONT_S, COL_DIM, b);

    box(s, 0, 286, UI_W, 100, COL_PANEL);
    float rem = c->progress.dist_remaining_m;
    if (rem >= 1000.0f) snprintf(b, sizeof b, "%.2f", (double)(rem / 1000.0f)); else snprintf(b, sizeof b, "%.0f", (double)rem);
    lbl_c(s, 0, 292, 130, FONT_XL, COL_TEXT, b);
    lbl_c(s, 0, 346, 130, FONT_S, COL_DIM, rem >= 1000.0f ? "KM RESTANTS" : "M RESTANTS");
    box(s, 130, 298, 1, 76, COL_LINE);
    snprintf(b, sizeof b, "%.1f%%", (double)c->progress.avg_grade_remaining_pct);
    lbl_c(s, 131, 302, 88, FONT_L, ui_level_color(level_of(c->progress.avg_grade_remaining_pct)), b);
    lbl_c(s, 131, 346, 88, FONT_S, COL_DIM, "PENTE MOY.");
    box(s, 220, 298, 1, 76, COL_LINE);
    snprintf(b, sizeof b, "%.0f m", (double)c->progress.ascent_remaining_m);
    lbl_c(s, 221, 302, 99, FONT_L, COL_TEXT, b);
    lbl_c(s, 221, 346, 99, FONT_S, COL_DIM, "D+ RESTANT");

    /* champs libres : FC, vitesse, cadence */
    if (d->hr >= 0) snprintf(b, sizeof b, "%d", d->hr); else snprintf(b, sizeof b, "--");
    lbl_c(s, 0, 394, 106, FONT_L, d->hr >= 0 ? COL_BAD : COL_DIM, b);
    lbl_c(s, 0, 428, 106, FONT_S, COL_DIM, "FC bpm");
    snprintf(b, sizeof b, "%.1f", (double)d->speed_kmh);
    lbl_c(s, 106, 394, 108, FONT_L, COL_TEXT, b);
    lbl_c(s, 106, 428, 108, FONT_S, COL_DIM, "km/h");
    if (d->cad >= 0) snprintf(b, sizeof b, "%d", d->cad); else snprintf(b, sizeof b, "--");
    lbl_c(s, 214, 394, 106, FONT_L, d->cad >= 0 ? COL_ACC : COL_DIM, b);
    lbl_c(s, 214, 428, 106, FONT_S, COL_DIM, "CAD rpm");

    /* montee suivante */
    box(s, 0, 450, UI_W, 30, COL_PANEL);
    if (c->next) {
        lv_obj_t *mini = box(s, 8, 457, 16, 16, ui_category_color(climb_category(c->next->score)));
        lv_obj_set_style_radius(mini, 4, 0);
        snprintf(b, sizeof b, "Suivante : montée %d dans %.1f km  (%.1f %%)", c->next_number, (double)(c->next_in_m / 1000.0f), (double)c->next->avg_grade_pct);
        lbl(s, 32, 458, FONT_S, COL_TEXT, b);
    } else {
        lbl(s, 12, 458, FONT_S, COL_DIM, "Dernière montée du parcours");
    }
    return s;
}

/* ---------- glisser pour changer de page ---------- */
static void pager_gesture(lv_event_t *e)
{
    ui_pager_t *p = (ui_pager_t *)lv_event_get_user_data(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT && p->cur < p->n - 1) ui_pager_goto(p, p->cur + 1, 1);
    else if (dir == LV_DIR_RIGHT && p->cur > 0) ui_pager_goto(p, p->cur - 1, -1);
}

void ui_pager_init(ui_pager_t *p) { p->n = 0; p->cur = 0; }

void ui_pager_add(ui_pager_t *p, lv_obj_t *screen)
{
    if (p->n >= 8) return;
    p->pages[p->n++] = screen;
    lv_obj_add_event_cb(screen, pager_gesture, LV_EVENT_GESTURE, p);
}

void ui_pager_goto(ui_pager_t *p, int index, int dir)
{
    if (index < 0 || index >= p->n) return;
    lv_screen_load_anim(p->pages[index], dir >= 0 ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
    p->cur = index;
}
