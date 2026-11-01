// Тесты формул и логики AlashBME280 без железа: g++ -I mock -I ../../src test_bme280.cpp ../../src/AlashBME280.cpp
#include <assert.h>
#include <stdio.h>
#include "Arduino.h"
#include "Wire.h"
#include "AlashBME280.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

int main() {
  // 1) BMP280: пример из datasheet Bosch: T = 25.08 °C, P = 100653.27 Па
  g_bme.load(false);
  AlashBME280 bmp;
  CHECK(bmp.begin());
  CHECK(bmp.chip() == AlashBME280::CHIP_BMP280);
  CHECK(!bmp.hasHumidity());
  CHECK(near(bmp.temperature(), 25.08f, 0.01f));
  CHECK(near(bmp.pressurePa(), 100653.27f, 3.0f));      // float хранит ~0.01 гПа
  CHECK(isnan(bmp.humidity()));
  CHECK(near(bmp.pressureMmHg(), 754.9f, 0.2f));
  CHECK(near(bmp.temperatureF(), 77.14f, 0.05f));

  // 2) высота: на уровне моря 1013.25 гПа = 0 м; ниже давление - выше высота
  float alt = bmp.altitude(1013.25f);
  CHECK(alt > 50 && alt < 70);                         // 1006.5 гПа -> ~55 м
  CHECK(near(bmp.seaLevelPressure(alt), 1013.25f, 0.2f));

  // 3) BME280: влажность в диапазоне, растёт с adc_H, точка росы ниже температуры
  g_bme.load(true);
  AlashBME280 bme;
  bme.setCacheMs(0);
  g_bme.adcH = 22000; CHECK(bme.begin()); CHECK(bme.chip() == AlashBME280::CHIP_BME280); float h1 = bme.humidity();
  g_bme.adcH = 26000; float h2 = bme.humidity();
  g_bme.adcH = 30000; float h3 = bme.humidity();
  printf("humidity: %.1f %.1f %.1f\n", h1, h2, h3);
  CHECK(h1 >= 0 && h3 <= 100 && h1 < h2 && h2 < h3);
  CHECK(bme.dewPoint() < bme.temperature());

  // 4) автоопределение адреса 0x77
  g_bme.load(true); g_bme.addr = 0x77;
  AlashBME280 b77; CHECK(b77.begin()); CHECK(b77.address() == 0x77);

  // 5) датчика нет: понятная ошибка и NAN
  g_bme.present = false;
  AlashBME280 none; CHECK(!none.begin()); CHECK(none.error() == AlashBME280::ERR_NO_DEVICE);
  CHECK(isnan(none.temperature()));
  none.setLanguage(AlashBME280::LANG_KK); printf("kk: %s\n", (const char *)none.errorText());
  none.setLanguage(AlashBME280::LANG_EN); printf("en: %s\n", (const char *)none.errorText());

  // 6) чужой чип на адресе
  g_bme.present = true; g_bme.addr = 0x76; g_bme.load(true); g_bme.reg[0xD0] = 0x61;  // BME680
  AlashBME280 wrong; CHECK(!wrong.begin()); CHECK(wrong.error() == AlashBME280::ERR_WRONG_CHIP);

  // 7) кэш: за 100 мс повторные чтения не мерят заново
  g_bme.load(true); g_bme.adcH = 26000; AlashBME280 c; c.begin();
  float t1 = c.temperature(); g_bme.adcT = 540000; float t2 = c.temperature();
  CHECK(t1 == t2);
  g_now += 150; float t3 = c.temperature(); CHECK(t3 > t1);

  printf(fails ? "\nТЕСТЫ НЕ ПРОШЛИ: %d\n" : "\nВсе тесты прошли\n", fails);
  return fails ? 1 : 0;
}
