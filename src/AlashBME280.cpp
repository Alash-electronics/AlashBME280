// AlashBME280: реализация. Формулы компенсации из документации Bosch (BME280 datasheet, раздел 4.2.3).
#include "AlashBME280.h"

namespace {
const uint8_t REG_DIG_T1 = 0x88;   // 0x88..0xA1: калибровка T, P и H1
const uint8_t REG_ID = 0xD0;
const uint8_t REG_RESET = 0xE0;
const uint8_t REG_DIG_H2 = 0xE1;   // 0xE1..0xE7: калибровка H2..H6
const uint8_t REG_CTRL_HUM = 0xF2;
const uint8_t REG_STATUS = 0xF3;
const uint8_t REG_CTRL_MEAS = 0xF4;
const uint8_t REG_CONFIG = 0xF5;
const uint8_t REG_DATA = 0xF7;     // 0xF7..0xFE: давление, температура, влажность
const uint8_t SOFT_RESET = 0xB6;
const uint8_t MODE_FORCED = 0x01;
}  // namespace

// ---- Запуск ----------------------------------------------------------------
bool AlashBME280::begin(TwoWire &wire, bool startWire) {
  _wire = &wire;
  if (startWire) _wire->begin();
  _err = ERR_NO_DEVICE;
  _chip = CHIP_NONE;
  Error firstError = ERR_NO_DEVICE;
  for (uint8_t a = 0x76; a <= 0x77; a++) {
    if (probe(a)) return true;
    if (_err == ERR_WRONG_CHIP) firstError = ERR_WRONG_CHIP;
  }
  _err = firstError;
  return false;
}

bool AlashBME280::begin(uint8_t address, TwoWire &wire, bool startWire) {
  _wire = &wire;
  if (startWire) _wire->begin();
  _chip = CHIP_NONE;
  return probe(address);
}

#if defined(ESP32) || defined(ESP8266)
bool AlashBME280::begin(int sdaPin, int sclPin) {
  Wire.begin(sdaPin, sclPin);
  return begin(Wire, false);
}
#endif

bool AlashBME280::probe(uint8_t address) {
  _addr = address;
  _wire->beginTransmission(address);
  if (_wire->endTransmission() != 0) {
    _err = ERR_NO_DEVICE;
    return false;
  }
  uint8_t id = 0;
  if (!read8(REG_ID, id)) {
    _err = ERR_I2C;
    return false;
  }
  if (id != CHIP_BME280 && id != CHIP_BMP280) {
    _err = ERR_WRONG_CHIP;
    return false;
  }
  _chip = static_cast<Chip>(id);
  if (!reset()) {
    _chip = CHIP_NONE;
    return false;
  }
  if (!readCalibration()) {
    _chip = CHIP_NONE;
    return false;
  }
  write8(REG_CONFIG, 0x00);  // фильтр выключен: в разовом режиме он не нужен
  _hasData = false;
  _err = ERR_OK;
  return true;
}

bool AlashBME280::reset() {
  if (_chip == CHIP_NONE) return false;
  if (!write8(REG_RESET, SOFT_RESET)) return false;
  delay(5);  // старт после сброса: около 2 мс
  uint8_t st = 0xFF;
  for (uint8_t i = 0; i < 20; i++) {  // ждём конца копирования калибровки (бит im_update)
    if (read8(REG_STATUS, st) && !(st & 0x01)) break;
    delay(2);
  }
  _err = ERR_OK;
  return true;
}

bool AlashBME280::isConnected() {
  if (_chip == CHIP_NONE) return false;
  uint8_t id = 0;
  return read8(REG_ID, id) && id == _chip;
}

// ---- Калибровка ------------------------------------------------------------
bool AlashBME280::readCalibration() {
  uint8_t b[26];
  if (!readBytes(REG_DIG_T1, b, 26)) return false;
  _T1 = (uint16_t)(b[1] << 8 | b[0]);
  _T2 = (int16_t)(b[3] << 8 | b[2]);
  _T3 = (int16_t)(b[5] << 8 | b[4]);
  _P1 = (uint16_t)(b[7] << 8 | b[6]);
  _P2 = (int16_t)(b[9] << 8 | b[8]);
  _P3 = (int16_t)(b[11] << 8 | b[10]);
  _P4 = (int16_t)(b[13] << 8 | b[12]);
  _P5 = (int16_t)(b[15] << 8 | b[14]);
  _P6 = (int16_t)(b[17] << 8 | b[16]);
  _P7 = (int16_t)(b[19] << 8 | b[18]);
  _P8 = (int16_t)(b[21] << 8 | b[20]);
  _P9 = (int16_t)(b[23] << 8 | b[22]);
  _H1 = b[25];
  if (_chip == CHIP_BME280) {
    uint8_t h[7];
    if (!readBytes(REG_DIG_H2, h, 7)) return false;
    _H2 = (int16_t)(h[1] << 8 | h[0]);
    _H3 = h[2];
    _H4 = (int16_t)((int8_t)h[3] * 16 | (h[4] & 0x0F));
    _H5 = (int16_t)((int8_t)h[5] * 16 | (h[4] >> 4));
    _H6 = (int8_t)h[6];
  }
  return true;
}

// ---- Настройки -------------------------------------------------------------
uint8_t AlashBME280::osrsCode(uint8_t samples) {
  if (samples <= 1) return 1;
  if (samples <= 2) return 2;
  if (samples <= 4) return 3;
  if (samples <= 8) return 4;
  return 5;
}

void AlashBME280::setOversampling(uint8_t temperature, uint8_t pressure, uint8_t humidity) {
  _osrsT = osrsCode(temperature);
  _osrsP = osrsCode(pressure);
  _osrsH = osrsCode(humidity);
  _hasData = false;
}

void AlashBME280::setPreset(Preset preset) {
  switch (preset) {
    case PRESET_PRECISE: setOversampling(16, 16, 16); break;
    case PRESET_FAST: setOversampling(1, 1, 1); break;
    case PRESET_WEATHER:
    default: setOversampling(2, 2, 2); break;
  }
}

// ---- Измерение ---------------------------------------------------------------
bool AlashBME280::update() {
  if (_chip == CHIP_NONE) return false;
  if (_chip == CHIP_BME280 && !write8(REG_CTRL_HUM, _osrsH)) return false;
  // ctrl_hum применяется только после записи ctrl_meas
  if (!write8(REG_CTRL_MEAS, (uint8_t)((_osrsT << 5) | (_osrsP << 2) | MODE_FORCED))) return false;

  // время измерения по datasheet: 1.25 + 2.3*T + (2.3*P + 0.575) + (2.3*H + 0.575), мс
  float ms = 1.25f + 2.3f * (1 << (_osrsT - 1)) + 2.3f * (1 << (_osrsP - 1)) + 0.575f;
  if (_chip == CHIP_BME280) ms += 2.3f * (1 << (_osrsH - 1)) + 0.575f;
  delay((uint32_t)ms + 1);

  uint8_t st = 0xFF;
  uint32_t t0 = millis();
  while (true) {  // ждём, пока датчик закончит измерение
    if (!read8(REG_STATUS, st)) return false;
    if (!(st & 0x08)) break;
    if (millis() - t0 > 200) {
      _err = ERR_TIMEOUT;
      return false;
    }
    delay(1);
  }

  uint8_t d[8];
  uint8_t n = (_chip == CHIP_BME280) ? 8 : 6;
  if (!readBytes(REG_DATA, d, n)) return false;
  int32_t adcP = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
  int32_t adcT = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
  int32_t adcH = (n == 8) ? (((int32_t)d[6] << 8) | d[7]) : 0x8000;
  compensate(adcT, adcP, adcH);
  _lastRead = millis();
  _hasData = true;
  _err = ERR_OK;
  return true;
}

void AlashBME280::compensate(int32_t adcT, int32_t adcP, int32_t adcH) {
  // температура (целочисленная формула Bosch)
  int32_t v1 = ((((adcT >> 3) - ((int32_t)_T1 << 1))) * ((int32_t)_T2)) >> 11;
  int32_t v2 = (((((adcT >> 4) - ((int32_t)_T1)) * ((adcT >> 4) - ((int32_t)_T1))) >> 12) * ((int32_t)_T3)) >> 14;
  _tFine = v1 + v2;
  _t = ((_tFine * 5 + 128) >> 8) / 100.0f;

  // давление (64-битная формула Bosch)
  int64_t p1 = (int64_t)_tFine - 128000;
  int64_t p2 = p1 * p1 * (int64_t)_P6;
  p2 = p2 + ((p1 * (int64_t)_P5) << 17);
  p2 = p2 + (((int64_t)_P4) << 35);
  p1 = ((p1 * p1 * (int64_t)_P3) >> 8) + ((p1 * (int64_t)_P2) << 12);
  p1 = (((((int64_t)1) << 47) + p1)) * ((int64_t)_P1) >> 33;
  if (p1 == 0 || adcP == 0x80000) {
    _p = NAN;
  } else {
    int64_t p = 1048576 - adcP;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = (((int64_t)_P9) * (p >> 13) * (p >> 13)) >> 25;
    p2 = (((int64_t)_P8) * p) >> 19;
    p = ((p + p1 + p2) >> 8) + (((int64_t)_P7) << 4);
    _p = (float)p / 256.0f;  // Па
  }

  // влажность (только BME280)
  if (_chip == CHIP_BME280 && adcH != 0x8000) {
    int32_t x = _tFine - ((int32_t)76800);
    x = (((((adcH << 14) - (((int32_t)_H4) << 20) - (((int32_t)_H5) * x)) + ((int32_t)16384)) >> 15) *
         (((((((x * ((int32_t)_H6)) >> 10) * (((x * ((int32_t)_H3)) >> 11) + ((int32_t)32768))) >> 10) +
             ((int32_t)2097152)) * ((int32_t)_H2) + 8192) >> 14));
    x = (x - (((((x >> 15) * (x >> 15)) >> 7) * ((int32_t)_H1)) >> 4));
    x = (x < 0 ? 0 : x);
    x = (x > 419430400 ? 419430400 : x);
    _h = (float)(x >> 12) / 1024.0f;
  } else {
    _h = NAN;
  }
}

bool AlashBME280::ensureFresh() {
  if (_chip == CHIP_NONE) return false;
  if (_hasData && _cacheMs > 0 && (uint32_t)(millis() - _lastRead) < _cacheMs) return true;
  return update();
}

// ---- Значения ----------------------------------------------------------------
float AlashBME280::temperature() { return ensureFresh() ? _t : NAN; }
float AlashBME280::temperatureF() { return ensureFresh() ? _t * 1.8f + 32.0f : NAN; }
float AlashBME280::humidity() { return ensureFresh() ? _h : NAN; }
float AlashBME280::pressurePa() { return ensureFresh() ? _p : NAN; }
float AlashBME280::pressure() { return pressurePa() / 100.0f; }
float AlashBME280::pressureMmHg() { return pressurePa() * 0.00750062f; }

float AlashBME280::altitude(float seaLevel_hPa) {
  float p = pressure();
  if (isnan(p) || seaLevel_hPa <= 0) return NAN;
  return 44330.0f * (1.0f - powf(p / seaLevel_hPa, 0.190294957f));
}

float AlashBME280::seaLevelPressure(float knownAltitude_m) {
  float p = pressure();
  if (isnan(p)) return NAN;
  return p / powf(1.0f - knownAltitude_m / 44330.0f, 5.255f);
}

float AlashBME280::dewPoint() {
  float t = temperature();
  float h = humidity();
  if (isnan(t) || isnan(h) || h <= 0) return NAN;
  const float a = 17.62f, b = 243.12f;  // формула Магнуса
  float g = logf(h / 100.0f) + (a * t) / (b + t);
  return b * g / (a - g);
}

// ---- I2C ---------------------------------------------------------------------
bool AlashBME280::read8(uint8_t reg, uint8_t &value) { return readBytes(reg, &value, 1); }

bool AlashBME280::readBytes(uint8_t reg, uint8_t *buf, uint8_t len) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) {
    _err = ERR_I2C;
    return false;
  }
  if (_wire->requestFrom(_addr, len) != len) {
    _err = ERR_I2C;
    return false;
  }
  for (uint8_t i = 0; i < len; i++) buf[i] = _wire->read();
  return true;
}

bool AlashBME280::write8(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(value);
  if (_wire->endTransmission() != 0) {
    _err = ERR_I2C;
    return false;
  }
  return true;
}

// ---- Ошибки и диагностика ----------------------------------------------------
const __FlashStringHelper *AlashBME280::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_NO_DEVICE:
      return _lang == LANG_KK ? F("Датчик табылмады: сымдарды, қуатты (3.3 В) және SDA/SCL пиндерін тексеріңіз")
             : _lang == LANG_EN ? F("Sensor not found: check wiring, 3.3 V power and the SDA/SCL pins")
                                : F("Датчик не найден: проверьте провода, питание 3.3 В и пины SDA/SCL");
    case ERR_WRONG_CHIP:
      return _lang == LANG_KK ? F("Бұл BME280/BMP280 емес (басқа чип жауап береді)")
             : _lang == LANG_EN ? F("This is not a BME280/BMP280 (another chip answers)")
                                : F("Это не BME280/BMP280 (отвечает другой чип)");
    case ERR_I2C:
      return _lang == LANG_KK ? F("I2C қатесі: сымдардың байланысын және тартқыш резисторларды тексеріңіз")
             : _lang == LANG_EN ? F("I2C error: check connections and pull-up resistors")
                                : F("Ошибка I2C: проверьте контакты и подтяжку линий");
    case ERR_TIMEOUT:
      return _lang == LANG_KK ? F("Датчик өлшеуді аяқтамады") : _lang == LANG_EN ? F("The sensor did not finish measuring")
                                                                              : F("Датчик не закончил измерение");
  }
  return F("");
}

void AlashBME280::diagnose(Print &out) {
  const bool ru = _lang == LANG_RU, kk = _lang == LANG_KK;
  out.println(kk ? F("AlashBME280: диагностика") : ru ? F("AlashBME280: диагностика") : F("AlashBME280: diagnostics"));
  out.println(kk ? F("I2C құрылғылары:") : ru ? F("Найденные устройства I2C:") : F("I2C devices found:"));
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    _wire->beginTransmission(a);
    if (_wire->endTransmission() == 0) {
      found++;
      out.print(F("  0x"));
      if (a < 16) out.print('0');
      out.print(a, HEX);
      if (a == 0x76 || a == 0x77) out.print(F("  BME280 / BMP280"));
      else if (a == 0x3C || a == 0x3D) out.print(F("  OLED SSD1306"));
      else if (a == 0x27 || a == 0x3F) out.print(F("  LCD I2C"));
      else if (a == 0x68) out.print(F("  MPU6050 / DS3231"));
      out.println();
    }
  }
  if (!found) out.println(kk ? F("  ештеңе жоқ: SDA/SCL және GND/VCC тексеріңіз") : ru ? F("  ничего нет: проверьте SDA/SCL и питание") : F("  nothing: check SDA/SCL and power"));
  if (_chip != CHIP_NONE) {
    out.print(kk ? F("Датчик: ") : ru ? F("Датчик: ") : F("Sensor: "));
    out.print(_chip == CHIP_BME280 ? F("BME280") : F("BMP280"));
    out.print(F(", 0x"));
    out.println(_addr, HEX);
    if (_chip == CHIP_BMP280)
      out.println(kk ? F("Назар: BMP280 ылғалдылықты өлшемейді") : ru ? F("Внимание: BMP280 не измеряет влажность") : F("Note: BMP280 has no humidity sensor"));
  } else {
    out.println(errorText());
  }
}
