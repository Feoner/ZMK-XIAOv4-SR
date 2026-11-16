/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/activity.h>
#include <zmk/events/activity_state_changed.h>
#include "sleep_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

LV_IMG_DECLARE(vim_32x128);
LV_IMG_DECLARE(vim_68x160);
LV_IMG_DECLARE(sleep_oled);
#if IS_ENABLED(CONFIG_NICE_OLED_SLEEP_ART_CATBUS)
LV_IMG_DECLARE(catbus_sleep);
#endif

// skreecustom: the weak default is replaced by animation.c on the peripheral half
__weak void nice_oled_animation_set_paused(bool paused) { ARG_UNUSED(paused); }

static void show_sleep_art(struct zmk_widget_sleep_status *widget, bool show) {
    // Hide/show the whole container, not just the image, so it never covers the
    // status screen while the keyboard is active.
    if (show) {
        lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(widget->obj);
    } else {
        lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    }
    nice_oled_animation_set_paused(show);
}

// skreecustom: upstream tested CONFIG_NICE_PERI_VIEW_* symbols that do not exist,
// so the art never appeared. Use this module's own Kconfig names.
static void set_sleep_img(struct zmk_widget_sleep_status *widget,
                          struct zmk_activity_state_changed ev) {
    switch (ev.state) {
    case ZMK_ACTIVITY_ACTIVE:
        LOG_DBG("ACTIVITY EVENT ACTIVE");
        show_sleep_art(widget, false);
        break;
    case ZMK_ACTIVITY_IDLE:
        LOG_DBG("ACTIVITY EVENT IDLE");
        show_sleep_art(widget, IS_ENABLED(CONFIG_NICE_OLED_SHOW_SLEEP_ART_ON_IDLE));
        break;
    case ZMK_ACTIVITY_SLEEP:
        LOG_DBG("ACTIVITY EVENT SLEEP");
        show_sleep_art(widget, IS_ENABLED(CONFIG_NICE_OLED_SHOW_SLEEP_ART_ON_SLEEP));
        break;
    default:
        LOG_WRN("Unhandled activity state: %d", ev.state);
    }
}

static void sleep_status_update_cb(struct zmk_activity_state_changed ev) {
    struct zmk_widget_sleep_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_sleep_img(widget, ev); }
}

static struct zmk_activity_state_changed sleep_status_get_state(const zmk_event_t *eh) {
    // skreecustom: ZMK calls this with eh == NULL at init; upstream dereferenced NULL here.
    struct zmk_activity_state_changed *ev = eh ? as_zmk_activity_state_changed(eh) : NULL;
    if (ev == NULL) {
        return (struct zmk_activity_state_changed){.state = zmk_activity_get_state()};
    }
    return *ev;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_sleep_status, struct zmk_activity_state_changed,
                            sleep_status_update_cb, sleep_status_get_state)

ZMK_SUBSCRIPTION(widget_sleep_status, zmk_activity_state_changed);

int zmk_widget_sleep_status_init(struct zmk_widget_sleep_status *widget, lv_obj_t *parent) {
    sys_slist_append(&widgets, &widget->node);
    widget->obj = lv_obj_create(parent);
    // skreecustom: no theme padding/border/scrollbar, so the art sits exactly at 0,0
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 160, 68);
    widget->art = lv_img_create(widget->obj);
#if IS_ENABLED(CONFIG_NICE_OLED_SLEEP_ART_CATBUS)
    lv_img_set_src(widget->art, &catbus_sleep);
#elif IS_ENABLED(CONFIG_NICE_EPAPER_ON)
    lv_img_set_src(widget->art, &vim_68x160);
#else
    // lv_img_set_src(widget->art, &sleep_oled);
    lv_img_set_src(widget->art, &vim_32x128);
#endif
    lv_obj_align(widget->art, LV_ALIGN_TOP_LEFT, 0, 0);

    // skreecustom: start hidden; shown only on idle/sleep
    lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);

    widget_sleep_status_init();
    return 0;
}

lv_obj_t *zmk_widget_sleep_status_obj(struct zmk_widget_sleep_status *widget) {
    return widget->obj;
}
