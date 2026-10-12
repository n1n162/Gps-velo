#ifndef UI_SCREENS_H
#define UI_SCREENS_H
/* Ecrans du GPS velo en portrait, 320 x 480 (ecran 3,5 pouces Waveshare ESP32-S3-Touch-LCD-3.5).
 * Inspires des pages ClimbPro (Garmin), Summit (Wahoo) et de la page de montees de COROS.
 * Navigation tactile : glisser a gauche ou a droite pour changer de page, toucher les boutons. */
#include "lvgl.h"
#include "climb.h"
#include "route.h"

LV_FONT_DECLARE(ui_font_14)
LV_FONT_DECLARE(ui_font_16)
LV_FONT_DECLARE(ui_font_20)
LV_FONT_DECLARE(ui_font_28)
LV_FONT_DECLARE(ui_font_48)

#define UI_W 320
#define UI_H 480

typedef struct {
    float speed_kmh, avg_kmh, dist_km;
    uint32_t time_s;
    int hr, cad;                 /* -1 = pas de capteur */
    float grade_pct, ascent_m, alt_m, route_left_km;
    int gps_ok, battery_pct, hour, minute;
    int page, pages;             /* points de page en haut */
    int paused;
} ui_data_t;

typedef struct {                 /* profil colore des prochains kilometres */
    const float *ele; int n_ele; float step_m;
    float pos_m, window_m;
    const climb_section_t *sections; int n_sections;
    const climb_t *climbs; int n_climbs;
    float alt_m, grade_now_pct, grade_ahead_pct, ascent_ahead_m;
} ui_elev_t;

typedef struct {                 /* ecran de montee en cours */
    int index, count;
    const climb_t *climb;
    const climb_section_t *sections; int n_sections;
    const float *ele; int n_ele; float step_m;
    float pos_m;
    climb_progress_t progress;
    float grade_now_pct, grade_ahead_pct;
    const climb_t *next; int next_number; float next_in_m;
} ui_climb_t;

typedef struct {                 /* trace */
    const route_t *route;
    float pos_m, view_m;
    const climb_t *next_climb;
    float next_climb_in_m, route_left_m;
} ui_nav_t;

typedef struct { int number; const climb_t *climb; float dist_to_start_m; int in_progress; } ui_list_row_t;
typedef struct {
    const ui_list_row_t *rows; int n_rows; int total;
    const float *ele; int n_ele; float step_m;
} ui_list_t;

/* Actions declenchees par le toucher : a brancher sur le reste du programme. */
typedef struct {
    void (*pause)(void);
    void (*lap)(void);
    void (*elev_zoom)(float window_m);
    void (*map_zoom)(int dir);       /* +1 = rapprocher, -1 = eloigner */
    void (*open_climb)(int number);
} ui_hooks_t;
extern ui_hooks_t ui_hooks;

/* Pages. Le profil des prochains km (strip) sert de petit apercu sur la page de donnees. */
lv_obj_t *ui_data_screen(const ui_data_t *d, const ui_elev_t *strip);
lv_obj_t *ui_nav_screen(const ui_nav_t *n, const ui_data_t *d);
lv_obj_t *ui_elev_screen(const ui_elev_t *e, const ui_data_t *d);
lv_obj_t *ui_list_screen(const ui_list_t *l, const ui_data_t *d);
lv_obj_t *ui_climb_screen(const ui_climb_t *c, const ui_data_t *d);

/* Changement de page au doigt (glisser a gauche = page suivante). */
typedef struct { lv_obj_t *pages[8]; int n, cur; } ui_pager_t;
void ui_pager_init(ui_pager_t *p);
void ui_pager_add(ui_pager_t *p, lv_obj_t *screen);
void ui_pager_goto(ui_pager_t *p, int index, int dir);

lv_color_t ui_level_color(int level);
lv_color_t ui_category_color(int category);
#endif
