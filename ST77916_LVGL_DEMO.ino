#include "scr_st77916.h"
#include <lvgl.h>
#include <demos/lv_demos.h>

// Simple struct for gauge configuration
typedef struct {
    const char *name;
    float min;
    float max;
    const char *unit;
} gauge_cfg_t;

static gauge_cfg_t gauges[] = {
    {"Boost",            0.0f,   1.5f,  "bar"},
    {"Oil Temp",         0.0f,   140.0f,"°C"},
    {"Water Temp",       0.0f,   140.0f,"°C"},
    {"IAT",             -20.0f, 100.0f,"°C"},
    {"Oil Pressure",     0.0f,   7.0f,  "bar"},
    {"RPM",              0.0f,   8000.0f,"rpm"}
};

static uint8_t current_gauge = 0;
static lv_obj_t *screen_objs[sizeof(gauges)/sizeof(gauges[0])];
static lv_indev_t *touch_indev = NULL;
static lv_point_t touch_start;
static bool touch_pressed = false;

static void create_gauge_screen(uint8_t idx) {
    gauge_cfg_t *cfg = &gauges[idx];
    lv_obj_t *scr = lv_obj_create(NULL);

    // Meter (analog gauge)
    lv_obj_t *meter = lv_meter_create(scr);
    lv_obj_set_size(meter, 260, 260);
    lv_obj_center(meter);

    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_range(meter, scale, (int)cfg->min, (int)cfg->max, 240, 135);
    lv_meter_set_scale_ticks(meter, scale, 31, 2, 10, lv_palette_main(LV_PALETTE_GREY));

    lv_meter_indicator_t *needle = lv_meter_add_needle_line(meter, scale, 15,
                                                            lv_palette_main(LV_PALETTE_RED), 0);

    // Title
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text_fmt(label, "%s", cfg->name);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Unit
    lv_obj_t *unit = lv_label_create(scr);
    lv_label_set_text_fmt(unit, "%s", cfg->unit);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Store screen
    screen_objs[idx] = scr;

    // For now, set a dummy value in mid‑range
    float mid = (cfg->min + cfg->max) * 0.5f;
    lv_meter_set_indicator_value(meter, needle, (int)mid);
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

// Simple swipe detection based on pointer events
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    LV_UNUSED(drv);

    // Use LVGL’s default pointer read; here we only implement swipe state machine stub.
    // In a full implementation, you would read CST816S coordinates and pressed state.

    // For now, leave data unchanged; swipe logic would be handled in an event callback
    // attached to the screens or meter object.
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
  vTaskDelay(5);
}
