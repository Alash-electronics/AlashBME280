# AlashBME280

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для датчиков **BME280** и **BMP280** (температура, влажность, давление, высота). Сделана для уроков и наборов Alash Electronics: один вызов `begin()`, понятные ошибки, без лишних зависимостей.

```cpp
#include <AlashBME280.h>
AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) { Serial.println(bme.errorText()); while (true); }
}
void loop() {
  Serial.println(bme.temperature());   // °C
  Serial.println(bme.humidity());      // %
  Serial.println(bme.pressure());      // гПа
  delay(2000);
}
```

### Что умеет
- **Сам находит датчик:** адрес 0x76 или 0x77 и тип (BME280 или BMP280). Не нужно знать адрес модуля.
- **Понятные ошибки** на русском, казахском и английском: `errorText()` и `diagnose(Serial)` (покажет все устройства на шине I2C и подскажет, что проверить).
- **Значения:** `temperature()`, `temperatureF()`, `humidity()`, `pressure()` (гПа), `pressurePa()`, `pressureMmHg()` (мм рт. ст.), `altitude()`, `seaLevelPressure()`, `dewPoint()` (точка росы).
- **Сам измеряет, когда нужно:** вызывать `update()` не надо; данные «свежие» 100 мс (`setCacheMs()`), датчик спит между измерениями и почти не греется.
- **Пресеты** точности: `PRESET_WEATHER` (по умолчанию), `PRESET_PRECISE`, `PRESET_FAST`, либо `setOversampling(t, p, h)`.
- **ESP32 / ESP8266:** `bme.begin(SDA, SCL)` для своих пинов.
- **Только встроенная библиотека `Wire`**, без Adafruit Unified Sensor и BusIO. Работает на Arduino AVR, ESP32, ESP8266 и других платах.
- BMP280 (без влажности) тоже поддерживается: `humidity()` вернёт `NAN`, `hasHumidity()` скажет `false`.

### Подключение (I2C)
| Датчик | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 3.3 В | 3.3 В |
| GND | GND | GND |
| SDA | A4 | GPIO 21 |
| SCL | A5 | GPIO 22 |

### Чем отличается от других библиотек
Мы посмотрели Adafruit BME280, SparkFun BME280, GyverBME280 и BME280 (Tyler Glenn):
- Adafruit по умолчанию ищет адрес 0x77 (у многих модулей 0x76) и требует две библиотеки-зависимости; SparkFun по умолчанию 0x77; GyverBME280 по умолчанию 0x76. В AlashBME280 адрес находится сам.
- В других нет подсказок, что делать, если датчик не нашёлся; у нас `errorText()` и `diagnose()`.
- Из коробки мм рт. ст., высота, точка росы и три языка.

### Примеры
`Basic`, `WeatherStation`, `Altitude`, `Diagnose`, `ESP32_CustomPins`.

### Тесты
Формулы проверены без железа на примере Bosch из документации (25,08 °C и 100653 Па): `extras/test`.

---

## Қазақша

**BME280** және **BMP280** сенсорларына (температура, ылғалдылық, қысым, биіктік) арналған қарапайым кітапхана. Alash Electronics сабақтары мен жинақтарына арналған: бір `begin()` шақыруы, түсінікті қателер, артық тәуелділіксіз.

```cpp
#include <AlashBME280.h>
AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) { Serial.println(bme.errorText()); while (true); }
}
void loop() {
  Serial.println(bme.temperature());   // °C
  Serial.println(bme.humidity());      // %
  Serial.println(bme.pressure());      // гПа
  delay(2000);
}
```

### Мүмкіндіктері
- **Сенсорды өзі табады:** 0x76 немесе 0x77 мекенжайы және түрі (BME280 не BMP280).
- **Түсінікті қателер** қазақша, орысша және ағылшынша: `errorText()` және `diagnose(Serial)` (I2C шинасындағы барлық құрылғыларды көрсетеді).
- **Мәндер:** `temperature()`, `humidity()`, `pressure()` (гПа), `pressureMmHg()`, `altitude()`, `dewPoint()` (шық нүктесі).
- **Тек кірістірілген `Wire` кітапханасы** қажет. Arduino AVR, ESP32, ESP8266 тақталарында жұмыс істейді.
- ESP32 / ESP8266: `bme.begin(SDA, SCL)` өз пиндеріңіз үшін.
- Тіл: `bme.setLanguage(AlashBME280::LANG_KK)`.

---

## English

An easy library for **BME280** and **BMP280** sensors (temperature, humidity, pressure, altitude). Made for classes and Alash Electronics kits: one `begin()` call, clear error messages, no extra dependencies.

```cpp
#include <AlashBME280.h>
AlashBME280 bme;

void setup() {
  Serial.begin(115200);
  if (!bme.begin()) { Serial.println(bme.errorText()); while (true); }
}
void loop() {
  Serial.println(bme.temperature());   // °C
  Serial.println(bme.humidity());      // %
  Serial.println(bme.pressure());      // hPa
  delay(2000);
}
```

### Features
- **Finds the sensor by itself:** address 0x76 or 0x77, and chip type (BME280 or BMP280).
- **Clear errors** in Russian, Kazakh and English: `errorText()` and `diagnose(Serial)` (lists every I2C device it can see).
- **Values:** `temperature()`, `humidity()`, `pressure()` (hPa), `pressureMmHg()`, `altitude()`, `seaLevelPressure()`, `dewPoint()`.
- **Only the built-in `Wire` library**, no Adafruit Unified Sensor or BusIO. Works on Arduino AVR, ESP32, ESP8266 and more.
- ESP32 / ESP8266: `bme.begin(SDA, SCL)` for custom pins. Language: `bme.setLanguage(AlashBME280::LANG_EN)`.

---

MIT License, Alash Electronics. Issues and ideas: https://github.com/Alash-electronics/AlashBME280/issues
