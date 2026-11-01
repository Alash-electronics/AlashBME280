// Имитация I2C и датчика BME280 (карта регистров) для тестов.
#pragma once
#include "Arduino.h"
struct FakeBme {
  bool present = true; uint8_t addr = 0x76; uint8_t id = 0x60;
  uint8_t reg[256] = {0};
  uint32_t adcT = 519888, adcP = 415148, adcH = 0x8000;
  bool measuring = false; int statusReads = 0;
  void load(bool bme280) {
    id = bme280 ? 0x60 : 0x58; memset(reg, 0, sizeof reg); reg[0xD0] = id;
    // калибровка из datasheet BMP280 (пример Bosch)
    const int16_t c[] = {(int16_t)27504, 26435, -1000, (int16_t)36477, -10685, 3024, 2855, 140, -7, 15500, -14600, 6000};
    for (int i = 0; i < 12; i++) { reg[0x88 + 2 * i] = c[i] & 0xFF; reg[0x89 + 2 * i] = (c[i] >> 8) & 0xFF; }
    reg[0xA1] = 75;                                   // H1
    reg[0xE1] = 0x70; reg[0xE2] = 0x01;               // H2 = 368
    reg[0xE3] = 0; reg[0xE4] = 0x13; reg[0xE5] = 0x2C; reg[0xE6] = 0x03; reg[0xE7] = 30;  // H3=0 H4=316 H5=50 H6=30
  }
};
inline FakeBme g_bme;
class TwoWire {
 public:
  void begin() {}
  void begin(int, int) {}
  void beginTransmission(uint8_t a) { _a = a; _n = 0; }
  size_t write(uint8_t v) { if (_n == 0) _reg = v; else if (_n == 1) onWrite(_reg, v); _n++; return 1; }
  uint8_t endTransmission(bool = true) { return (g_bme.present && _a == g_bme.addr) ? 0 : 2; }
  uint8_t requestFrom(uint8_t a, uint8_t len) {
    if (!(g_bme.present && a == g_bme.addr)) return 0;
    for (uint8_t i = 0; i < len; i++) _buf[i] = readReg(_reg + i);
    _len = len; _pos = 0; return len;
  }
  int read() { return _pos < _len ? _buf[_pos++] : -1; }
 private:
  uint8_t _a = 0, _reg = 0, _n = 0, _buf[32], _len = 0, _pos = 0;
  void onWrite(uint8_t r, uint8_t v) {
    if (r == 0xE0 && v == 0xB6) { return; }
    if (r == 0xF4) {
      g_bme.reg[0xF4] = v; if ((v & 3) == 1) { g_bme.measuring = true; g_bme.statusReads = 0;
        uint32_t p = g_bme.adcP << 4, t = g_bme.adcT << 4;
        g_bme.reg[0xF7] = p >> 16; g_bme.reg[0xF8] = p >> 8; g_bme.reg[0xF9] = p;
        g_bme.reg[0xFA] = t >> 16; g_bme.reg[0xFB] = t >> 8; g_bme.reg[0xFC] = t;
        g_bme.reg[0xFD] = g_bme.adcH >> 8; g_bme.reg[0xFE] = g_bme.adcH; }
      return; }
    g_bme.reg[r] = v;
  }
  uint8_t readReg(uint8_t r) {
    if (r == 0xF3) { if (g_bme.measuring && ++g_bme.statusReads > 1) g_bme.measuring = false; return g_bme.measuring ? 0x08 : 0x00; }
    return g_bme.reg[r];
  }
};
inline TwoWire Wire;
