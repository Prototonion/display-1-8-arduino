#include "scr_st77916.h"
#include <lvgl.h>
#include <demos/lv_demos.h>
#include <math.h>

// Gauge configuration
typedef struct {
    const char *name;
    float min;
    float max;
    lv_color_t bg_color;
    lv_color_t needle_color;
} gauge_cfg_t;

static gauge_cfg_t gauges[] = {
    {"Boost",            0.0f,   1.5f,   lv_color_black(), lv_palette_main(LV_PALETTE_RED)},
    {"Oil Temp",         0.0f,   140.0f, lv_color_black(), lv_palette_main(LV_PALETTE_ORANGE)},
    {"Water Temp",       0.0f,   140.0f, lv_color_black(), lv_palette_main(LV_PALETTE_BLUE)},
    {"IAT",             -20.0f,  100.0f, lv_color_black(), lv_palette_main(LV_PALETTE_GREEN)},
    {"Oil Pressure",     0.0f,   7.0f,   lv_color_black(), lv_palette_main(LV_PALETTE_PURPLE)},
    {"RPM",              0.0f,   8000.0f,lv_color_black(), lv_palette_main(LV_PALETTE_RED)}
};

static uint8_t current_gauge = 0;
static lv_obj_t *screen_objs[sizeof(gauges)/sizeof(gauges[0])];

// For dynamic demo values
static lv_obj_t *value_labels[sizeof(gauges)/sizeof(gauges[0])];
static lv_meter_indicator_t *needles[sizeof(gauges)/sizeof(gauges[0])];
static lv_meter_scale_t *scales[sizeof(gauges)/sizeof(gauges[0])];
static lv_obj_t *meters[sizeof(gauges)/sizeof(gauges[0])];

// Gesture callback oparty na LV_EVENT_GESTURE
static void scr_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
        if(dir == LV_DIR_LEFT) {
            uint8_t next = (current_gauge + 1) % count;
            current_gauge = next;
            lv_scr_load(screen_objs[next]);   // bez animacji, płynne przełączenie
        } else if(dir == LV_DIR_RIGHT) {
            uint8_t prev = (current_gauge + count - 1) % count;
            current_gauge = prev;
            lv_scr_load(screen_objs[prev]);   // bez animacji
        }
    }
}

static void create_gauge_screen(uint8_t idx) {
    gauge_cfg_t *cfg = &gauges[idx];
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, cfg->bg_color, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Full‑screen meter (dial)
    lv_obj_t *meter = lv_meter_create(scr);
    lv_obj_set_size(meter, 360, 360);   // pełny ekran 360x360
    lv_obj_center(meter);
    meters[idx] = meter;

    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    scales[idx] = scale;

    // Skala od 135° do 45° (240° łuku)
    lv_meter_set_scale_range(meter, scale, (int)cfg->min, (int)cfg->max, 240, 135);
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_darken(LV_PALETTE_GREY, 2));

    // Wskazówka 3px szerokości
    lv_meter_indicator_t *needle = lv_meter_add_needle_line(meter, scale, 3,
                                                            cfg->needle_color, 0);
    needles[idx] = needle;

    // Aktualna wartość (tekst)
    lv_obj_t *val_lbl = lv_label_create(scr);
    lv_label_set_text(val_lbl, "---");
    lv_obj_align(val_lbl, LV_ALIGN_CENTER, 0, 90);
    value_labels[idx] = val_lbl;

    // Cyfry przy podziałkach – własne etykiety, mniej gęste i poza łukiem
    const int label_cnt = 7;   // mniej wartości, mniejsze ryzyko nachodzenia
    const float angle_start = 135.0f;
    const float angle_range = 240.0f;
    const float radius = 175.0f;  // trochę poza łukiem skali
    const int cx = 180;
    const int cy = 180;

    for(int i = 0; i < label_cnt; i++) {
        float v = cfg->min + (cfg->max - cfg->min) * (float)i / (float)(label_cnt - 1);
        float angle_deg = angle_start + (v - cfg->min) * angle_range / (cfg->max - cfg->min);
        float rad = angle_deg * 3.14159265f / 180.0f;
        int x = cx + (int)(radius * cosf(rad));
        int y = cy + (int)(radius * sinf(rad));

        int display_val;
        if(idx == 5) {
            // RPM – skala x1000
            display_val = (int)(v / 1000.0f);
        } else {
            display_val = (int)v;
        }

        lv_obj_t *lbl = lv_label_create(scr);
        lv_label_set_text_fmt(lbl, "%d", display_val);
        lv_obj_set_pos(lbl, x, y);
    }

    // Store screen
    screen_objs[idx] = scr;

    // Początkowa wartość w środku zakresu
    float mid = (cfg->min + cfg->max) * 0.5f;
    lv_meter_set_indicator_value(meter, needle, (int)mid);
    lv_label_set_text_fmt(val_lbl, "%0.1f", mid);

    // Gesty na tym ekranie
    lv_obj_add_event_cb(scr, scr_event_cb, LV_EVENT_GESTURE, NULL);
}

static void build_all_screens(void) {
    uint8_t count = sizeof(gauges)/sizeof(gauges[0]);
    for(uint8_t i = 0; i < count; i++) {
        create_gauge_screen(i);
    }
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

        lv_meter_set_indicator_value(meters[i], needles[i], (int)value);
        lv_label_set_text_fmt(value_labels[i], "%0.1f", value);
    }
}

void setup()
{
  delay(200);
  Serial.begin(115200);
  scr_lvgl_init();

  build_all_screens();
  current_gauge = 0;
  lv_scr_load(screen_objs[0]);   // bez animacji, jak w prostych przykładach LVGL
}

void loop()
{
  lv_timer_handler();
  demo_update_values();
  vTaskDelay(20);
}
