// AlashBME280: простой датчик BME280 / BMP280 (температура, влажность, давление).
// MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashBME280
#ifndef ALASH_BME280_H
#define ALASH_BME280_H

#include <Arduino.h>
#include <Wire.h>

class AlashBME280 {
 public:
  // Какой чип найден
  enum Chip : uint8_t { CHIP_NONE = 0x00, CHIP_BMP280 = 0x58, CHIP_BME280 = 0x60 };

  // Что пошло не так (для errorText() и diagnose())
  enum Error : uint8_t {
    ERR_OK = 0,
    ERR_NO_DEVICE,    // на шине I2C нет ответа по адресам 0x76 и 0x77
    ERR_WRONG_CHIP,   // отвечает другое устройство (не BME280/BMP280)
    ERR_I2C,          // ошибка чтения или записи по I2C
    ERR_TIMEOUT       // датчик не закончил измерение
  };

  // Готовые настройки точности и скорости
  enum Preset : uint8_t {
    PRESET_WEATHER,   // по умолчанию: разовые измерения, x2, мало нагрева, хватает для метеостанции
    PRESET_PRECISE,   // x16 по всем каналам, медленнее, меньше шума (например, высота)
    PRESET_FAST       // x1, самое быстрое измерение
  };

  // Язык подсказок diagnose() и errorText()
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  // ---- Запуск --------------------------------------------------------------
  // begin() сам ищет датчик по адресам 0x76 и 0x77 и сам определяет BME280 или BMP280.
  // startWire = false: если вы сами уже вызвали Wire.begin(...) (например, со своими пинами).
  bool begin(TwoWire &wire = Wire, bool startWire = true);
  bool begin(uint8_t address, TwoWire &wire = Wire, bool startWire = true);
#if defined(ESP32) || defined(ESP8266)
  bool begin(int sdaPin, int sclPin);  // свои пины I2C (ESP32 / ESP8266)
#endif

  // ---- Измерения (при первом вызове за цикл датчик измеряет сам) --------------
  float temperature();     // °C
  float temperatureF();    // °F
  float humidity();        // %, у BMP280 вернёт NAN
  float pressure();        // гПа (hPa)
  float pressurePa();      // Па
  float pressureMmHg();    // мм рт. ст.
  float altitude(float seaLevel_hPa = 1013.25f);  // высота над уровнем моря, м
  float dewPoint();        // точка росы, °C (нужна влажность)
  // Давление, приведённое к уровню моря, по известной высоте места (для метеосводок)
  float seaLevelPressure(float knownAltitude_m);

  // Принудительно сделать новое измерение (обычно вызывать не нужно)
  bool update();

  // ---- Настройки -----------------------------------------------------------
  void setPreset(Preset preset);
  // oversampling: 1, 2, 4, 8 или 16 (чем больше, тем точнее и дольше)
  void setOversampling(uint8_t temperature, uint8_t pressure, uint8_t humidity);
  void setCacheMs(uint16_t ms) { _cacheMs = ms; }  // как долго считаются «свежими» последние данные (0 = всегда мерить)
  void setLanguage(Language lang) { _lang = lang; }
  bool reset();            // программный сброс датчика

  // ---- Состояние и диагностика ----------------------------------------------
  bool isConnected();
  Chip chip() const { return _chip; }
  bool hasHumidity() const { return _chip == CHIP_BME280; }
  uint8_t address() const { return _addr; }
  Error error() const { return _err; }
  const __FlashStringHelper *errorText();        // понятное описание последней ошибки (текст во флеш-памяти)
  void diagnose(Print &out);                     // печатает подсказки и список найденных I2C-устройств

 private:
  TwoWire *_wire = &Wire;
  uint8_t _addr = 0;
  Chip _chip = CHIP_NONE;
  Error _err = ERR_OK;
  Language _lang = LANG_RU;
  uint8_t _osrsT = 2, _osrsP = 2, _osrsH = 2;  // коды 0..5 (0 = выкл.)
  uint16_t _cacheMs = 100;
  uint32_t _lastRead = 0;
  bool _hasData = false;

  // калибровка из датчика
  uint16_t _T1 = 0, _P1 = 0;
  int16_t _T2 = 0, _T3 = 0, _P2 = 0, _P3 = 0, _P4 = 0, _P5 = 0, _P6 = 0, _P7 = 0, _P8 = 0, _P9 = 0;
  uint8_t _H1 = 0, _H3 = 0;
  int16_t _H2 = 0, _H4 = 0, _H5 = 0;
  int8_t _H6 = 0;

  // последние значения
  float _t = NAN, _p = NAN, _h = NAN;
  int32_t _tFine = 0;

  bool probe(uint8_t address);
  bool readCalibration();
  bool read8(uint8_t reg, uint8_t &value);
  bool readBytes(uint8_t reg, uint8_t *buf, uint8_t len);
  bool write8(uint8_t reg, uint8_t value);
  bool ensureFresh();
  static uint8_t osrsCode(uint8_t samples);
  void compensate(int32_t adcT, int32_t adcP, int32_t adcH);
};

#endif  // ALASH_BME280_H
