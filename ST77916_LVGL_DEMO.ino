#include "scr_st77916.h"
#include <lvgl.h>
#include <demos/lv_demos.h>

// Gauge configuration
typedef struct {
    const char *name;
    float min;
    float max;
    const char *unit;
    lv_color_t bg_color;
    lv_color_t needle_color;
} gauge_cfg_t;

static gauge_cfg_t gauges[] = {
    {"Boost",            0.0f,   1.5f,   "bar", lv_color_black(), lv_palette_main(LV_PALETTE_RED)},
    {"Oil Temp",         0.0f,   140.0f, "°C",  lv_color_black(), lv_palette_main(LV_PALETTE_ORANGE)},
    {"Water Temp",       0.0f,   140.0f, "°C",  lv_color_black(), lv_palette_main(LV_PALETTE_BLUE)},
    {"IAT",             -20.0f,  100.0f, "°C",  lv_color_black(), lv_palette_main(LV_PALETTE_GREEN)},
    {"Oil Pressure",     0.0f,   7.0f,   "bar", lv_color_black(), lv_palette_main(LV_PALETTE_PURPLE)},
    {"RPM",              0.0f,   8000.0f,"rpm", lv_color_black(), lv_palette_main(LV_PALETTE_RED)}
};

static uint8_t current_gauge = 0;
static lv_obj_t *screen_objs[sizeof(gauges)/sizeof(gauges[0])];

// For dynamic demo values
static lv_obj_t *value_labels[sizeof(gauges)/sizeof(gauges[0])];
static lv_meter_indicator_t *needles[sizeof(gauges)/sizeof(gauges[0])];
static lv_meter_scale_t *scales[sizeof(gauges)/sizeof(gauges[0])];

static void create_gauge_screen(uint8_t idx) {
    gauge_cfg_t *cfg = &gauges[idx];
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, cfg->bg_color, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Full‑screen meter (dial)
    lv_obj_t *meter = lv_meter_create(scr);
    lv_obj_set_size(meter, 340, 340);   // prawie cały 360x360, zostawia margines
    lv_obj_center(meter);

    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    scales[idx] = scale;

    // Skala od 135° do 45° (240° łuku)
    lv_meter_set_scale_range(meter, scale, (int)cfg->min, (int)cfg->max, 240, 135);
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_darken(LV_PALETTE_GREY, 2));
    lv_meter_set_scale_major_ticks(meter, scale, 9, 4, 20,
                                  lv_palette_darken(LV_PALETTE_GREY, 1), 10);

    // Wskazówka 3px szerokości
    lv_meter_indicator_t *needle = lv_meter_add_needle_line(meter, scale, 3,
                                                            cfg->needle_color, 0);
    needles[idx] = needle;

    // Title
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text_fmt(label, "%s", cfg->name);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Unit
    lv_obj_t *unit = lv_label_create(scr);
    lv_label_set_text_fmt(unit, "%s", cfg->unit);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Aktualna wartość (tekst)
    lv_obj_t *val_lbl = lv_label_create(scr);
    lv_label_set_text(val_lbl, "---");
    lv_obj_align(val_lbl, LV_ALIGN_CENTER, 0, 90);
    value_labels[idx] = val_lbl;

    // Początkowa wartość w środku zakresu
    float mid = (cfg->min + cfg->max) * 0.5f;
    lv_meter_set_indicator_value(meter, needle, (int)mid);
    lv_label_set_text_fmt(val_lbl, "%0.1f", mid);
}

static void build_all_screens(void) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    for(uint8_t i = 0; i < count; i++) {
        create_gauge_screen(i);
    }
}

static void load_gauge(uint8_t idx, lv_scr_load_anim_t anim) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    if(idx >= count) return;
    current_gauge = idx;
    lv_scr_load_anim(screen_objs[idx], anim, 300, 0, false);
}

static void swipe_next(void) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    uint8_t next = (current_gauge + 1) % count;
    load_gauge(next, LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

static void swipe_prev(void) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    uint8_t prev = (current_gauge + count - 1) % count;
    load_gauge(prev, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}

// Prosta generacja wartości demo (później zastąpisz ESP‑NOW)
static void demo_update_values(void) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    static uint32_t t = 0;
    t++;

    for(uint8_t i = 0; i < count; i++) {
        gauge_cfg_t *cfg = &gauges[i];
        float span = cfg->max - cfg->min;
        float phase = (float)((t + i * 50) % 1000) / 1000.0f;
        float value = cfg->min + span * phase;  // prosty przebieg w zakresie min..max

        lv_meter_set_indicator_value(lv_obj_get_child(screen_objs[i], 0), needles[i], (int)value);
        lv_label_set_text_fmt(value_labels[i], "%0.1f", value);
    }
}

void setup()
{
  delay(200);
  Serial.begin(115200);
  scr_lvgl_init();

  build_all_screens();
  load_gauge(0, LV_SCR_LOAD_ANIM_NONE);
}

void loop()
{
  lv_timer_handler();
  demo_update_values();
  vTaskDelay(20);
}
