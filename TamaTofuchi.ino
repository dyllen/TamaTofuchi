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
// SquareLine Studio UI includes (uncomment when your generated files are added)
// -----------------------------------------------------------------------------
#if defined(__has_include)
#if __has_include("ui.h")
#include "ui.h"
#define HAS_SQUARELINE_UI 1
#else
#define HAS_SQUARELINE_UI 0
#endif
#else
#define HAS_SQUARELINE_UI 0
#endif

// -----------------------------------------------------------------------------
// LVGL draw buffer
// -----------------------------------------------------------------------------
static lv_color_t lvgl_buf[SCREEN_W * 20];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static uint32_t last_tick_ms = 0;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  const int32_t x1 = area->x1;
  const int32_t y1 = area->y1;
  const int32_t x2 = area->x2;
  const int32_t y2 = area->y2;

  uint32_t i = 0;
  for (int32_t y = y1; y <= y2; y++) {
    for (int32_t x = x1; x <= x2; x++) {
      display.writePixel(x, y, color_p[i++].full);
    }
  }
  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);

  Serial.println("SSD1351 SPI mapping: SCL->SCK, SDA->MOSI");
  SPI.begin(TFT_SCL, -1, TFT_SDA, TFT_CS);
  display.begin();
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

#if HAS_SQUARELINE_UI
  ui_init();
#else
  // Fallback content so screen is visibly active even without generated UI.
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "LVGL ready");
  lv_obj_center(label);
#endif

  last_tick_ms = millis();
}

void loop() {
  uint32_t now = millis();
  lv_tick_inc(now - last_tick_ms);
  last_tick_ms = now;

  lv_timer_handler();
  delay(5);
}
