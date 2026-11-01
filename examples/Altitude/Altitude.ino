// AlashBME280: высота над уровнем моря по давлению.
// Давление зависит от погоды, поэтому сначала укажите высоту места, где вы находитесь:
// библиотека пересчитает давление «на уровень моря» и дальше покажет подъём или спуск.
#include <AlashBME280.h>

AlashBME280 bme;
const float MY_PLACE_ALTITUDE_M = 600;   // например, Алматы около 800 м, Астана около 350 м
float seaLevel_hPa = 1013.25f;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) {
    Serial.println(bme.errorText());
    while (true) delay(1000);
  }
  bme.setPreset(AlashBME280::PRESET_PRECISE);    // точнее: x16, подходит для высоты
  seaLevel_hPa = bme.seaLevelPressure(MY_PLACE_ALTITUDE_M);
  Serial.print("Давление на уровне моря: ");
  Serial.print(seaLevel_hPa, 1);
  Serial.println(" гПа");
}

void loop() {
  Serial.print("Высота: ");
  Serial.print(bme.altitude(seaLevel_hPa), 1);
  Serial.println(" м");
  delay(1000);
}
