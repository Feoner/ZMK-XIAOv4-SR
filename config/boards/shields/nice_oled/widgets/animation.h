#pragma once

#include <lvgl.h>
#include "util.h"
#include "screen_peripheral.h"

void draw_animation(lv_obj_t *canvas, struct zmk_widget_screen *widget);

// skreecustom: stop/restart the peripheral animation (used while sleep art is shown)
void nice_oled_animation_set_paused(bool paused);
