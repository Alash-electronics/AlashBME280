// AlashBME280: датчик не работает? Этот скетч покажет, что видит плата на шине I2C.
#include <AlashBME280.h>

AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  delay(500);
  bme.setLanguage(AlashBME280::LANG_RU);   // LANG_KK: қазақша, LANG_EN: English
  bool ok = bme.begin();
  Serial.println(ok ? "Датчик найден" : "Датчик НЕ найден");
  bme.diagnose(Serial);                    // список устройств I2C и подсказки
}

void loop() {}
