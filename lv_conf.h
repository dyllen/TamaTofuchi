#ifndef LV_CONF_H
#define LV_CONF_H

/*
 * Project-wide LVGL overrides for SquareLine-generated sources.
 * This header is picked up by LVGL in every translation unit (e.g. ui.c,
 * ui_helpers.c, and the sketch), so settings here are consistent everywhere.
 */

#ifndef LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif

/*
 * SquareLine-generated helpers use the LVGL v8 call shape
 *   lv_event_send(obj, code, param)
 * When building with LVGL v9, remap it to lv_obj_send_event(...) so generated
 * ui_helpers.c/ui_helpers.h compile without edits.
 */
#ifndef lv_event_send
#define lv_event_send(obj, code, param) lv_obj_send_event((obj), (code), (param))
#endif

#endif /* LV_CONF_H */
