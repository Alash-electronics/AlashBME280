// AlashBME280: все значения сразу: температура, влажность, давление (гПа и мм рт. ст.), точка росы.
#include <AlashBME280.h>

AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) {
    Serial.println(bme.errorText());
    while (true) delay(1000);
  }
  Serial.print("Найден датчик: ");
  Serial.println(bme.chip() == AlashBME280::CHIP_BME280 ? "BME280" : "BMP280 (без влажности)");
}

void loop() {
  Serial.print("T = ");        Serial.print(bme.temperature(), 1);   Serial.print(" °C   ");
  if (bme.hasHumidity()) {
    Serial.print("Влажность = "); Serial.print(bme.humidity(), 0);    Serial.print(" %   ");
    Serial.print("Точка росы = "); Serial.print(bme.dewPoint(), 1);   Serial.print(" °C   ");
  }
  Serial.print("P = ");        Serial.print(bme.pressure(), 1);      Serial.print(" гПа (");
  Serial.print(bme.pressureMmHg(), 0);                               Serial.println(" мм рт. ст.)");
  delay(2000);
}
