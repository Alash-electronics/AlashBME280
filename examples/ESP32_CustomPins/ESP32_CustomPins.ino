// AlashBME280 на ESP32 / ESP8266: датчик на своих пинах I2C.
#include <AlashBME280.h>

AlashBME280 bme;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);
  if (!bme.begin(SDA_PIN, SCL_PIN)) {      // сам запустит I2C на этих пинах
    Serial.println(bme.errorText());
    bme.diagnose(Serial);
    while (true) delay(1000);
  }
}

void loop() {
  Serial.printf("%.1f °C, %.0f %%, %.1f гПа\n", bme.temperature(), bme.humidity(), bme.pressure());
  delay(2000);
}
