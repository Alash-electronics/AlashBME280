// AlashBME280: самый простой пример.
// Подключение (I2C): VCC -> 3.3 В, GND -> GND, SDA -> SDA, SCL -> SCL
//   Arduino Uno/Nano: SDA = A4, SCL = A5;  ESP32: SDA = 21, SCL = 22
#include <AlashBME280.h>

AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) {                 // сам найдёт датчик по адресу 0x76 или 0x77
    Serial.println(bme.errorText());  // понятно объяснит, что не так
    while (true) delay(1000);
  }
}

void loop() {
  Serial.print("Температура: ");  Serial.print(bme.temperature());  Serial.println(" °C");
  Serial.print("Влажность: ");    Serial.print(bme.humidity());     Serial.println(" %");
  Serial.print("Давление: ");     Serial.print(bme.pressure());     Serial.println(" гПа");
  Serial.println();
  delay(2000);
}
