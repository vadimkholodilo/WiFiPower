#ifndef __RTCDS1302_H
#define __RTCDS1302_H

#include "Rtc.h"

class RtcDS1302 : public RtcBase {
public:
  RtcDS1302(uint8_t pinRst, uint8_t pinDat, uint8_t pinClk);
  virtual bool begin();
  virtual void get(uint8_t& hour, uint8_t& minute, uint8_t& second, uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow);
  virtual void getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow);
  virtual void getTime(uint8_t& hour, uint8_t& minute, uint8_t& second);
  virtual uint8_t getHour();
  virtual uint8_t getMinute();
  virtual uint8_t getSecond();
  virtual uint16_t getYear();
  virtual uint8_t getMonth();
  virtual uint8_t getDay();
  virtual uint8_t getDow();
  virtual void set(uint8_t hour, uint8_t minute, uint8_t second, uint16_t year, uint8_t month, uint8_t day, uint8_t dow);
  virtual void setDate(uint16_t year, uint8_t month, uint8_t day, uint8_t dow);
  virtual void setTime(uint8_t hour, uint8_t minute, uint8_t second);
  virtual void setHour(uint8_t hour);
  virtual void setMinute(uint8_t minute);
  virtual void setSecond(uint8_t second);
  virtual void setYear(uint16_t year);
  virtual void setMonth(uint8_t month);
  virtual void setDay(uint8_t day);
  virtual void setDow(uint8_t dow);
protected:
  void _burstread(uint8_t *p);
  void _burstwrite(uint8_t *p);
  uint8_t _read(int address);
  void _write(int address, uint8_t data);
  void _start();
  void _stop();
  uint8_t _toggleread();
  void _togglewrite(uint8_t data, uint8_t release);

  uint8_t _pinRst, _pinDat, _pinClk;
};

#endif
