#include "RtcDS1302.h"

// DS1302 RTC
// ----------
//
// Open Source / Public Domain
//
// Version 1
//     By arduino.cc user "Krodal".
//     June 2012
//     Using Arduino 1.0.1
// Version 2
//     By arduino.cc user "Krodal"
//     March 2013
//     Using Arduino 1.0.3, 1.5.2
//     The code is no longer compatible with older versions.
//     Added bcd2dec, dec2bcd_h, dec2bcd_l
//     A few minor changes.
//
//
// Documentation: datasheet
// 
// The DS1302 uses a 3-wire interface: 
//    - bidirectional data.
//    - clock
//    - chip select
// It is not I2C, not OneWire, and not SPI.
// So the standard libraries can not be used.
// Even the shiftOut() function is not used, since it
// could be too fast (it might be slow enough, 
// but that's not certain).
//
// I wrote my own interface code according to the datasheet.
// Any three pins of the Arduino can be used.
//   See the first defines below this comment, 
//   to set your own pins.
//
// The "Chip Enable" pin was called "/Reset" before.
//
// The chip has internal pull-down registers.
// This keeps the chip disabled, even if the pins of 
// the Arduino are floating.
//
//
// Range
// -----
//      seconds : 00-59
//      minutes : 00-59
//      hour    : 1-12 or 0-23
//      date    : 1-31
//      month   : 1-12
//      day     : 1-7
//      year    : 00-99
//
//
// Burst mode
// ----------
// In burst mode, all the clock data is read at once.
// This is to prevent a rollover of a digit during reading.
// The read data is from an internal buffer.
//
// The burst registers are commands, rather than addresses.
// Clock Data Read in Burst Mode
//    Start by writing 0xBF (as the address), 
//    after that: read clock data
// Clock Data Write in Burst Mode
//    Start by writing 0xBE (as the address), 
//    after that: write clock data
// Ram Data Read in Burst Mode
//    Start by writing 0xFF (as the address), 
//    after that: read ram data
// Ram Data Write in Burst Mode
//    Start by writing 0xFE (as the address), 
//    after that: write ram data
//
//
// Ram
// ---
// The DS1302 has 31 of ram, which can be used to store data.
// The contents will be lost if the Arduino is off, 
// and the backup battery gets empty.
// It is better to store data in the EEPROM of the Arduino.
// The burst read or burst write for ram is not implemented 
// in this code.
//
//
// Trickle charge
// --------------
// The DS1302 has a build-in trickle charger.
// That can be used for example with a lithium battery 
// or a supercap.
// Using the trickle charger has not been implemented 
// in this code.
//

// Macros to convert the bcd values of the registers to normal
// integer variables.
// The code uses seperate variables for the high byte and the low byte
// of the bcd, so these macros handle both bytes seperately.
#define bcd2dec(h,l)  (((h)*10) + (l))
#define dec2bcd_h(x)  ((x)/10)
#define dec2bcd_l(x)  ((x)%10)

// Register names.
// Since the highest bit is always '1', 
// the registers start at 0x80
// If the register is read, the lowest bit should be '1'.
#define DS1302_SECONDS           0x80
#define DS1302_MINUTES           0x82
#define DS1302_HOURS             0x84
#define DS1302_DATE              0x86
#define DS1302_MONTH             0x88
#define DS1302_DAY               0x8A
#define DS1302_YEAR              0x8C
#define DS1302_ENABLE            0x8E
#define DS1302_TRICKLE           0x90
#define DS1302_CLOCK_BURST       0xBE
#define DS1302_CLOCK_BURST_WRITE 0xBE
#define DS1302_CLOCK_BURST_READ  0xBF
#define DS1302_RAMSTART          0xC0
#define DS1302_RAMEND            0xFC
#define DS1302_RAM_BURST         0xFE
#define DS1302_RAM_BURST_WRITE   0xFE
#define DS1302_RAM_BURST_READ    0xFF

// Defines for the bits, to be able to change 
// between bit number and binary definition.
// By using the bit number, using the DS1302 
// is like programming an AVR microcontroller.
// But instead of using "(1<<X)", or "_BV(X)", 
// the Arduino "bit(X)" is used.
#define DS1302_D0 0
#define DS1302_D1 1
#define DS1302_D2 2
#define DS1302_D3 3
#define DS1302_D4 4
#define DS1302_D5 5
#define DS1302_D6 6
#define DS1302_D7 7

// Bit for reading (bit in address)
#define DS1302_READBIT DS1302_D0 // READBIT=1: read instruction

// Bit for clock (0) or ram (1) area, 
// called R/C-bit (bit in address)
#define DS1302_RC DS1302_D6

// Seconds Register
#define DS1302_CH DS1302_D7   // 1 = Clock Halt, 0 = start

// Hour Register
#define DS1302_AM_PM DS1302_D5 // 0 = AM, 1 = PM
#define DS1302_12_24 DS1302_D7 // 0 = 24 hour, 1 = 12 hour

// Enable Register
#define DS1302_WP DS1302_D7   // 1 = Write Protect, 0 = enabled

// Trickle Register
#define DS1302_ROUT0 DS1302_D0
#define DS1302_ROUT1 DS1302_D1
#define DS1302_DS0   DS1302_D2
#define DS1302_DS1   DS1302_D2
#define DS1302_TCS0  DS1302_D4
#define DS1302_TCS1  DS1302_D5
#define DS1302_TCS2  DS1302_D6
#define DS1302_TCS3  DS1302_D7

// Structure for the first 8 registers.
// These 8 bytes can be read at once with 
// the 'clock burst' command.
// Note that this structure contains an anonymous union.
// It might cause a problem on other compilers.
struct ds1302_struct {
  uint8_t Seconds : 4;      // low decimal digit 0-9
  uint8_t Seconds10 : 3;    // high decimal digit 0-5
  uint8_t CH : 1;           // CH = Clock Halt
  uint8_t Minutes : 4;
  uint8_t Minutes10 : 3;
  uint8_t reserved1 : 1;
  union {
    struct {
      uint8_t Hour : 4;
      uint8_t Hour10 : 2;
      uint8_t reserved2 : 1;
      uint8_t hour_12_24 : 1; // 0 for 24 hour format
    } h24;
    struct {
      uint8_t Hour : 4;
      uint8_t Hour10 : 1;
      uint8_t AM_PM : 1;      // 0 for AM, 1 for PM
      uint8_t reserved2 : 1;
      uint8_t hour_12_24 : 1; // 1 for 12 hour format
    } h12;
  };
  uint8_t Date : 4;           // Day of month, 1 = first day
  uint8_t Date10 : 2;
  uint8_t reserved3 : 2;
  uint8_t Month : 4;          // Month, 1 = January
  uint8_t Month10 : 1;
  uint8_t reserved4 : 3;
  uint8_t Day : 3;            // Day of week, 1 = first day (any day)
  uint8_t reserved5 : 5;
  uint8_t Year : 4;           // Year, 0 = year 2000
  uint8_t Year10 : 4;
  uint8_t reserved6 : 7;
  uint8_t WP : 1;             // WP = Write Protect
};

/***
 * RtcDS1302 class implementation
 */

RtcDS1302::RtcDS1302(uint8_t pinRst, uint8_t pinDat, uint8_t pinClk) {
  _pinRst = pinRst;
  _pinDat = pinDat;
  _pinClk = pinClk;
}

bool RtcDS1302::begin() {
  // Disable Trickle Charger
  _write(DS1302_TRICKLE, 0x00);

  return true;
}

void RtcDS1302::get(uint8_t& hour, uint8_t& minute, uint8_t& second, uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
  _burstread((uint8_t *)&rtc);
  hour = bcd2dec(rtc.h24.Hour10, rtc.h24.Hour);
  minute = bcd2dec(rtc.Minutes10, rtc.Minutes);
  second = bcd2dec(rtc.Seconds10, rtc.Seconds);
  year = bcd2dec(rtc.Year10, rtc.Year) + 2000;
  month = bcd2dec(rtc.Month10, rtc.Month);
  day = bcd2dec(rtc.Date10, rtc.Date);
  dow = rtc.Day;
}

void RtcDS1302::getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& dow) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
  _burstread((uint8_t *)&rtc);
  year = bcd2dec(rtc.Year10, rtc.Year) + 2000;
  month = bcd2dec(rtc.Month10, rtc.Month);
  day = bcd2dec(rtc.Date10, rtc.Date);
  dow = rtc.Day;
}

void RtcDS1302::getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
  _burstread((uint8_t *)&rtc);
  hour = bcd2dec(rtc.h24.Hour10, rtc.h24.Hour);
  minute = bcd2dec(rtc.Minutes10, rtc.Minutes);
  second = bcd2dec(rtc.Seconds10, rtc.Seconds);
}

inline uint8_t RtcDS1302::getHour() {
  return bcd2bin(_read(DS1302_HOURS));
}

inline uint8_t RtcDS1302::getMinute() {
  return bcd2bin(_read(DS1302_MINUTES));
}

inline uint8_t RtcDS1302::getSecond() {
  return bcd2bin(_read(DS1302_SECONDS));
}

inline uint16_t RtcDS1302::getYear() {
  return bcd2bin(_read(DS1302_YEAR)) + 2000;
}

inline uint8_t RtcDS1302::getMonth() {
  return bcd2bin(_read(DS1302_MONTH));
}

inline uint8_t RtcDS1302::getDay() {
  return bcd2bin(_read(DS1302_DATE));
}

inline uint8_t RtcDS1302::getDow() {
  return _read(DS1302_DAY);
}

void RtcDS1302::set(uint8_t hour, uint8_t minute, uint8_t second, uint16_t year, uint8_t month, uint8_t day, uint8_t dow) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
//  _burstread((uint8_t *)&rtc);
  rtc.Seconds = dec2bcd_l(second);
  rtc.Seconds10 = dec2bcd_h(second);
  rtc.Minutes = dec2bcd_l(minute);
  rtc.Minutes10 = dec2bcd_h(minute);
  rtc.h24.Hour = dec2bcd_l(hour);
  rtc.h24.Hour10 = dec2bcd_h(hour);
  rtc.h24.hour_12_24 = 0; // 0 for 24 hour format
  rtc.Date = dec2bcd_l(day);
  rtc.Date10 = dec2bcd_h(day);
  rtc.Month = dec2bcd_l(month);
  rtc.Month10 = dec2bcd_h(month);
  rtc.Year = dec2bcd_l(year - 2000);
  rtc.Year10 = dec2bcd_h(year - 2000);
  rtc.Day = dow;
  rtc.CH = 0; // 1 for Clock Halt, 0 to run;
  rtc.WP = 0;
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  // Write all clock data at once (burst mode)
  _burstwrite((uint8_t *)&rtc);
}

void RtcDS1302::setDate(uint16_t year, uint8_t month, uint8_t day, uint8_t dow) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
  _burstread((uint8_t *)&rtc);
  rtc.Date = dec2bcd_l(day);
  rtc.Date10 = dec2bcd_h(day);
  rtc.Month = dec2bcd_l(month);
  rtc.Month10 = dec2bcd_h(month);
  rtc.Year = dec2bcd_l(year - 2000);
  rtc.Year10 = dec2bcd_h(year - 2000);
  rtc.Day = dow;
  rtc.CH = 0; // 1 for Clock Halt, 0 to run;
  rtc.WP = 0;
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  // Write all clock data at once (burst mode)
  _burstwrite((uint8_t *)&rtc);
}

void RtcDS1302::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
  ds1302_struct rtc;

  // Read all clock data at once (burst mode)
  _burstread((uint8_t *)&rtc);
  rtc.Seconds = dec2bcd_l(second);
  rtc.Seconds10 = dec2bcd_h(second);
  rtc.Minutes = dec2bcd_l(minute);
  rtc.Minutes10 = dec2bcd_h(minute);
  rtc.h24.Hour = dec2bcd_l(hour);
  rtc.h24.Hour10 = dec2bcd_h(hour);
  rtc.h24.hour_12_24 = 0; // 0 for 24 hour format
  rtc.CH = 0; // 1 for Clock Halt, 0 to run;
  rtc.WP = 0;
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  // Write all clock data at once (burst mode)
  _burstwrite((uint8_t *)&rtc);
}

inline void RtcDS1302::setHour(uint8_t hour) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_HOURS, bin2bcd(hour));
}

inline void RtcDS1302::setMinute(uint8_t minute) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_MINUTES, bin2bcd(minute));
}

inline void RtcDS1302::setSecond(uint8_t second) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_SECONDS, bin2bcd(second));
}

inline void RtcDS1302::setYear(uint16_t year) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_YEAR, bin2bcd(year - 2000));
}

inline void RtcDS1302::setMonth(uint8_t month) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_MONTH, bin2bcd(month));
}

inline void RtcDS1302::setDay(uint8_t day) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_DATE, bin2bcd(day));
}

inline void RtcDS1302::setDow(uint8_t dow) {
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  _write(DS1302_ENABLE, 0x00);
  _write(DS1302_DAY, dow);
}

void RtcDS1302::_burstread(uint8_t *p) {
  _start();
  // Instead of the address, 
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  _togglewrite(DS1302_CLOCK_BURST_READ, true);
  for (uint8_t i = 0; i < 8; i++) {
    *p++ = _toggleread();
  }
  _stop();
}

void RtcDS1302::_burstwrite(uint8_t *p) {
  _start();
  // Instead of the address, 
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  _togglewrite(DS1302_CLOCK_BURST_WRITE, false);
  for (uint8_t i = 0; i < 8; i++) {
    // the I/O-line is not released
    _togglewrite(*p++, false);
  }
  _stop();
}

uint8_t RtcDS1302::_read(int address) {
  uint8_t data;

  // set lowest bit (read bit) in address
  bitSet(address, DS1302_READBIT);
  _start();
  // the I/O-line is released for the data
  _togglewrite(address, true);
  data = _toggleread();
  _stop();

  return data;
}

void RtcDS1302::_write(int address, uint8_t data) {
  // clear lowest bit (read bit) in address
  bitClear(address, DS1302_READBIT);
  _start();
  // don't release the I/O-line
  _togglewrite(address, false);
  // don't release the I/O-line
  _togglewrite(data, false);
  _stop();
}

void RtcDS1302::_start() {
  digitalWrite(_pinRst, LOW); // default, not enabled
  pinMode(_pinRst, OUTPUT);

  digitalWrite(_pinClk, LOW); // default, clock low
  pinMode(_pinClk, OUTPUT);

  pinMode(_pinDat, OUTPUT);

  digitalWrite(_pinRst, HIGH); // start the session
  delayMicroseconds(4);       // tCC = 4us
}

void RtcDS1302::_stop() {
  // Set CE low
  digitalWrite(_pinRst, LOW);

  delayMicroseconds(4);       // tCWH = 4us
}

uint8_t RtcDS1302::_toggleread() {
  uint8_t data = 0;

  for (uint8_t i = 0; i <= 7; i++) {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before 
    // this function, the SCLK is already high.
    digitalWrite(_pinClk, HIGH);
    delayMicroseconds(1);
    // Clock down, data is ready after some time.
    digitalWrite(_pinClk, LOW);
    delayMicroseconds(1);        // tCL=1000ns, tCDD=800ns
    // read bit, and set it in place in 'data' variable
    bitWrite(data, i, digitalRead(_pinDat));
  }

  return data;
}

void RtcDS1302::_togglewrite(uint8_t data, uint8_t release) {
  for (uint8_t i = 0; i <= 7; i++) {
    // set a bit of the data on the I/O-line
    digitalWrite(_pinDat, bitRead(data, i));
    delayMicroseconds(1);     // tDC = 200ns
    // clock up, data is read by DS1302
    digitalWrite(_pinClk, HIGH);
    delayMicroseconds(1);     // tCH = 1000ns, tCDH = 800ns
    if (release && (i == 7)) {
      // If this write is followed by a read, 
      // the I/O-line should be released after 
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release 
      // the I/O-line at this moment, 
      // and that could cause a shortcut spike 
      // on the I/O-line.
      pinMode(_pinDat, INPUT);
      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite(DS1302_IO, LOW); // remove any pull-up
    } else {
      digitalWrite(_pinClk, LOW);
      delayMicroseconds(1);       // tCL=1000ns, tCDD=800ns
    }
  }
}
