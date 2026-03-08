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

  last_tick_ms = millis();
}

void loop() {
  uint32_t now = millis();
  lv_tick_inc(now - last_tick_ms);
  last_tick_ms = now;

  lv_timer_handler();
  delay(5);
}
