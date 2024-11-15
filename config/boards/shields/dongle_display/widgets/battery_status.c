/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include "battery_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct peripheral_battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};
    
// static lv_color_t battery_image_buffer[ZMK_SPLIT_BLE_PERIPHERAL_COUNT][9 * 15];

static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // inverted colors
    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_fill_dsc;
    lv_draw_rect_dsc_init(&rect_fill_dsc);
    rect_fill_dsc.bg_color = lv_color_black();

    lv_canvas_draw_rect(canvas, 0, 0, 5, level / 3, &rect_fill_dsc);
}

static void set_battery_symbol(lv_obj_t *widget, struct peripheral_battery_state state) {
    lv_obj_t *symbol = lv_obj_get_child(widget, state.source * 2);
    lv_obj_t *label = lv_obj_get_child(widget, state.source * 2 + 1);

    // draw_battery(symbol, state.level);
    // if (state.usb_present) {
    //     lv_label_set_text_fmt(label, "+%3u%%", state.level);
    // } else {
    lv_label_set_text_fmt(label, "%3u", state.level);
    // }
    
    if (state.level > 0) {
        lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
}

void battery_status_update_cb(struct peripheral_battery_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct peripheral_battery_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct peripheral_battery_state){
        .source = ev->source,
        .level = ev->state_of_charge,
        .usb_present = zmk_usb_is_powered(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct peripheral_battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_peripheral_battery_state_changed);

int zmk_widget_peripheral_battery_status_init(struct zmk_widget_peripheral_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        // lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        // lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 9, 15, LV_IMG_CF_TRUE_COLOR);
        // lv_obj_align(image_canvas, LV_ALIGN_TOP_RIGHT, i * 10, 0);

        lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, i * 10, 0);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_peripheral_battery_status_obj(struct zmk_widget_peripheral_battery_status *widget) {
    return widget->obj;
}
