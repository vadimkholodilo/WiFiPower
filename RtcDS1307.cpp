#include <Wire.h>
#include "RtcDS1307.h"

#define DS1307_ADDRESS    0x68 // I2C Slave address

#define DS1307_SEC_REG    0x00
#define DS1307_MIN_REG    0x01
#define DS1307_HOUR_REG   0x02
#define DS1307_WDAY_REG   0x03
#define DS1307_MDAY_REG   0x04
#define DS1307_MONTH_REG  0x05
#define DS1307_YEAR_REG   0x06

#define DS1307_CONTROL_REG  0x07

/***
 * RtcDS1307 class implementation
 */

bool RtcDS1307::begin() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_CONTROL_REG);
  Wire.write((byte)0x00);
  byte status = Wire.endTransmission();

  return (status == 0);
}

void RtcDS1307::get(uint8_t& hour, uint8_t& minute, uint8_t& second, uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_SEC_REG);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  second = bcd2bin(Wire.read());
  minute = bcd2bin(Wire.read());
  hour = bcd2bin(Wire.read() & ~0b11000000); // Ignore 24 Hour bit
  dow = Wire.read();
  day = bcd2bin(Wire.read());
  month = bcd2bin(Wire.read());
  year = bcd2bin(Wire.read()) + 2000;
}

void RtcDS1307::getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_WDAY_REG);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 4);
  dow = Wire.read();
  day = bcd2bin(Wire.read());
  month = bcd2bin(Wire.read());
  year = bcd2bin(Wire.read()) + 2000;
}

void RtcDS1307::getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_SEC_REG);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 3);
  second = bcd2bin(Wire.read());
  minute = bcd2bin(Wire.read());
  hour = bcd2bin(Wire.read() & ~0b11000000); // Ignore 24 Hour bit
}

inline uint8_t RtcDS1307::getHour() {
  return bcd2bin(_read(DS1307_HOUR_REG) & ~0b11000000); // Ignore 24 Hour bit
}

inline uint8_t RtcDS1307::getMinute() {
  return bcd2bin(_read(DS1307_MIN_REG));
}

inline uint8_t RtcDS1307::getSecond() {
  return bcd2bin(_read(DS1307_SEC_REG));
}

inline uint16_t RtcDS1307::getYear() {
  return bcd2bin(_read(DS1307_YEAR_REG)) + 2000;
}

inline uint8_t RtcDS1307::getMonth() {
  return bcd2bin(_read(DS1307_MONTH_REG));
}

inline uint8_t RtcDS1307::getDay() {
  return bcd2bin(_read(DS1307_MDAY_REG));
}

inline uint8_t RtcDS1307::getDow() {
  return _read(DS1307_WDAY_REG);
}

void RtcDS1307::set(uint8_t hour, uint8_t minute, uint8_t second, uint16_t year, uint8_t month, uint8_t day, uint8_t dow) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_SEC_REG);  // beginning from SEC Register address

  Wire.write((byte)bin2bcd(second));
  Wire.write((byte)bin2bcd(minute));
  Wire.write((byte)bin2bcd(hour));
  Wire.write((byte)dow);
  Wire.write((byte)bin2bcd(day));
  Wire.write((byte)bin2bcd(month));
  Wire.write((byte)bin2bcd(year - 2000));
  Wire.endTransmission();
}

void RtcDS1307::setDate(uint16_t year, uint8_t month, uint8_t day, uint8_t dow) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_WDAY_REG);

  Wire.write((byte)dow);
  Wire.write((byte)bin2bcd(day));
  Wire.write((byte)bin2bcd(month));
  Wire.write((byte)bin2bcd(year - 2000));
  Wire.endTransmission();
}

void RtcDS1307::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)DS1307_SEC_REG);  // beginning from SEC Register address

  Wire.write((byte)bin2bcd(second));
  Wire.write((byte)bin2bcd(minute));
  Wire.write((byte)bin2bcd(hour));
  Wire.endTransmission();
}

inline void RtcDS1307::setHour(uint8_t hour) {
  _write(DS1307_HOUR_REG, bin2bcd(hour));
}

inline void RtcDS1307::setMinute(uint8_t minute) {
  _write(DS1307_MIN_REG, bin2bcd(minute));
}

inline void RtcDS1307::setSecond(uint8_t second) {
  _write(DS1307_SEC_REG, bin2bcd(second));
}

inline void RtcDS1307::setYear(uint16_t year) {
  _write(DS1307_YEAR_REG, bin2bcd(year - 2000));
}

inline void RtcDS1307::setMonth(uint8_t month) {
  _write(DS1307_MONTH_REG, bin2bcd(month));
}

inline void RtcDS1307::setDay(uint8_t day) {
  _write(DS1307_MDAY_REG, bin2bcd(day));
}

inline void RtcDS1307::setDow(uint8_t dow) {
  _write(DS1307_WDAY_REG, dow);
}

uint8_t RtcDS1307::_read(byte address) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)address);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);

  return Wire.read();
}

void RtcDS1307::_write(byte address, byte value) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte)address);
  Wire.write((byte)value);
  Wire.endTransmission();
}
