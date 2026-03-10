#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

// Match SquareLine Studio's LVGL color byte-order setting.
#ifndef LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif

#include <lvgl.h>

// -----------------------------------------------------------------------------
// Display + pinout (same wiring as before)
// -----------------------------------------------------------------------------
static constexpr uint8_t TFT_CS  = 3;
static constexpr uint8_t TFT_DC  = 4;
static constexpr uint8_t TFT_RST = 5;

// On many SSD1351 breakouts these are labeled SCL/SDA, but in SPI mode they map to:
//   display SCL -> SPI SCK (clock)
//   display SDA -> SPI MOSI (data out from MCU)
static constexpr uint8_t TFT_SCL  = 8;   // SPI clock (SCK)
static constexpr uint8_t TFT_SDA  = 10;  // SPI MOSI

static constexpr uint16_t SCREEN_W = 128;
static constexpr uint16_t SCREEN_H = 128;

Adafruit_SSD1351 display(SCREEN_W, SCREEN_H, &SPI, TFT_CS, TFT_DC, TFT_RST);

// -----------------------------------------------------------------------------
// SquareLine Studio generated UI
// -----------------------------------------------------------------------------
#include "ui.h"
#include "ui_helpers.h"
#include "ui_events.h"

#define BTN_NEXT 2
#define BTN_SELECT 21
#define BTN_PAT 20

lv_group_t *menuGroup = nullptr;

bool lastNextState = HIGH;
bool lastSelectState = HIGH;
bool lastPatState = HIGH;

unsigned long lastNextPressTime = 0;
unsigned long lastSelectPressTime = 0;
unsigned long lastPatPressTime = 0;

const unsigned long debounceMs = 180;

static lv_obj_t *trackedScreen = nullptr;
static int32_t preferredFocusIndex = -1;

static bool isMenuFocusable(lv_obj_t *obj)
{
    if (obj == nullptr) return false;

    if (!lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;

    return true;
}

static void addFocusableChildrenToGroup(
    lv_obj_t *parent,
    lv_group_t *group,
    lv_obj_t **firstAdded,
    lv_obj_t **preferredObj,
    int32_t *focusableIndex)
{
    if (parent == nullptr || group == nullptr || focusableIndex == nullptr) return;

    uint32_t childCount = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);

        if (isMenuFocusable(child)) {
            lv_group_add_obj(group, child);
            if (*firstAdded == nullptr) {
                *firstAdded = child;
            }

            if (*focusableIndex == preferredFocusIndex && preferredObj != nullptr) {
                *preferredObj = child;
            }

            (*focusableIndex)++;
        }

        addFocusableChildrenToGroup(child, group, firstAdded, preferredObj, focusableIndex);
    }
}

static bool findFocusableIndex(lv_obj_t *parent, lv_obj_t *target, int32_t *focusableIndex, int32_t *matchIndex)
{
    if (parent == nullptr || target == nullptr || focusableIndex == nullptr || matchIndex == nullptr) return false;

    uint32_t childCount = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);

        if (isMenuFocusable(child)) {
            if (child == target) {
                *matchIndex = *focusableIndex;
                return true;
            }
            (*focusableIndex)++;
        }

        if (findFocusableIndex(child, target, focusableIndex, matchIndex)) {
            return true;
        }
    }

    return false;
}

static int32_t getFocusableIndex(lv_obj_t *screen, lv_obj_t *target)
{
    int32_t focusableIndex = 0;
    int32_t matchIndex = -1;

    findFocusableIndex(screen, target, &focusableIndex, &matchIndex);
    return matchIndex;
}

void initMenuGroup()
{
    lv_obj_t *activeScreen = lv_scr_act();
    if (activeScreen == nullptr) {
        menuGroup = nullptr;
        trackedScreen = nullptr;
        return;
    }

    if (menuGroup != nullptr) {
        lv_group_del(menuGroup);
        menuGroup = nullptr;
    }

    menuGroup = lv_group_create();
    trackedScreen = activeScreen;

    lv_obj_t *firstFocusable = nullptr;
    lv_obj_t *preferredFocusable = nullptr;
    int32_t focusableIndex = 0;
    addFocusableChildrenToGroup(activeScreen, menuGroup, &firstFocusable, &preferredFocusable, &focusableIndex);

    if (preferredFocusable != nullptr) {
        lv_group_focus_obj(preferredFocusable);
    } else if (firstFocusable != nullptr) {
        lv_group_focus_obj(firstFocusable);
        preferredFocusIndex = 0;
    }
}

void ensureMenuGroupIsCurrent()
{
    lv_obj_t *activeScreen = lv_scr_act();
    if (menuGroup == nullptr || trackedScreen != activeScreen) {
        initMenuGroup();
    }
}

void handleNextButton()
{
    Serial.println("NEXT button pressed");
    ensureMenuGroupIsCurrent();

    if (menuGroup == NULL)
    {
        Serial.println("menuGroup is NULL");
        return;
    }

    lv_group_focus_next(menuGroup);
    lv_obj_t *focused = lv_group_get_focused(menuGroup);
    preferredFocusIndex = getFocusableIndex(lv_scr_act(), focused);
    Serial.println("Focus moved to next object");
}

void handleSelectButton()
{
    ensureMenuGroupIsCurrent();

    if(menuGroup == NULL) return;

    lv_obj_t *focused = lv_group_get_focused(menuGroup);

    if (focused == nullptr) return;

    preferredFocusIndex = getFocusableIndex(lv_scr_act(), focused);

    lv_obj_t *screenBeforeClick = lv_scr_act();
    lv_event_send(focused, LV_EVENT_CLICKED, NULL);

    if (lv_scr_act() == screenBeforeClick && lv_obj_is_valid(focused)) {
        lv_group_focus_obj(focused);
    }
}

void handlePatButton()
{
    if (lv_scr_act() != ui_Screen3 || !lv_obj_is_valid(ui_Image11)) return;

    lv_anim_del(ui_Image11, _ui_anim_callback_set_image_frame);
    pat_Animation(ui_Image11, 0);
}

void pollPhysicalButtons()
{
    bool currentNextState = digitalRead(BTN_NEXT);
    bool currentSelectState = digitalRead(BTN_SELECT);
    bool currentPatState = digitalRead(BTN_PAT);

    unsigned long now = millis();

    // Detect new press on NEXT button
    if (lastNextState == HIGH && currentNextState == LOW)
    {
        if (now - lastNextPressTime > debounceMs)
        {
            handleNextButton();
            lastNextPressTime = now;
        }
    }

    // Detect new press on SELECT button
    if (lastSelectState == HIGH && currentSelectState == LOW)
    {
        if (now - lastSelectPressTime > debounceMs)
        {
            handleSelectButton();
            lastSelectPressTime = now;
        }
    }

    // Detect new press on PAT button
    if (lastPatState == HIGH && currentPatState == LOW)
    {
        if (now - lastPatPressTime > debounceMs)
        {
            handlePatButton();
            lastPatPressTime = now;
        }
    }

    lastNextState = currentNextState;
    lastSelectState = currentSelectState;
    lastPatState = currentPatState;
}

// -----------------------------------------------------------------------------
// LVGL draw buffer
// -----------------------------------------------------------------------------
static lv_color_t lvgl_buf[SCREEN_W * 20];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static uint32_t last_tick_ms = 0;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  const int16_t x1 = static_cast<int16_t>(area->x1);
  const int16_t y1 = static_cast<int16_t>(area->y1);
  const int16_t w = static_cast<int16_t>(area->x2 - area->x1 + 1);
  const int16_t h = static_cast<int16_t>(area->y2 - area->y1 + 1);

  // Stream the LVGL dirty area directly into the SSD1351 GRAM window.
  display.startWrite();
  display.setAddrWindow(x1, y1, w, h);
  const uint32_t px_count = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
  for (uint32_t i = 0; i < px_count; ++i) {
    uint16_t color = color_p[i].full;
#if LV_COLOR_16_SWAP
    // LVGL buffer stores swapped bytes; SSD1351 expects normal RGB565 order.
    color = static_cast<uint16_t>((color >> 8) | (color << 8));
#endif
    display.writeColor(color, 1);
  }
  display.endWrite();

  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);

  Serial.println("SSD1351 SPI mapping: SCL->SCK, SDA->MOSI");
  SPI.begin(TFT_SCL, -1, TFT_SDA, TFT_CS);
  display.begin();
  // Rotate the panel -90° at boot so UI orientation matches the device layout.
  display.setRotation(3);
  // Hardware sanity check: briefly flash white so you can confirm panel power.
  display.fillScreen(0xFFFF);
  delay(120);
  display.fillScreen(0x0000);

  lv_init();

  lv_disp_draw_buf_init(&draw_buf, lvgl_buf, nullptr, SCREEN_W * 20);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  ui_init();

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_PAT, INPUT_PULLUP);
  initMenuGroup();
  Serial.println("Menu group initialized");

  last_tick_ms = millis();
}

void loop() {
  uint32_t now = millis();
  lv_tick_inc(now - last_tick_ms);
  last_tick_ms = now;

  lv_timer_handler();
  pollPhysicalButtons();
  delay(5);
}
