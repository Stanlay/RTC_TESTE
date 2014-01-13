//-----------------------------------------------//
// RTC DS1307 LIBRARY FOR SPARK CORE             //
//===============================================//
// Copy this into a new application at:          //
// https://www.spark.io/build and go nuts!       //
// !! Pinouts on line 254 below !!               //
//-----------------------------------------------//
// Technobly / BDub - Jan 12th 2014              //
//===============================================//
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsequence-point"

#define DS1307_ADDRESS 0xD0 // Switch back to 0x68 when I2C 7-bit addressing is fixed
#define SECONDS_PER_DAY 86400L
#define SECONDS_FROM_1970_TO_2000 946684800

/* =========================== RTC.h =============================== */
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const char* date, const char* time);
    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;   
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// RTC based on the DS1307 chip connected via I2C and the Wire library
class RTC_DS1307 {
public:
    static uint8_t begin(void);
    static void adjust(const DateTime& dt);
    uint8_t isrunning(void);
    static DateTime now();
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)
class RTC_Millis {
public:
    static void adjust(const DateTime& dt);
    static DateTime now();

protected:
    static long offset;
};

/* =========================== RTC.cpp =============================== */
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!



////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth[13] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
  if (y >= 2000)
    y -= 2000;
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i)
    days += daysInMonth[i - 1];
  if (m > 2 && y % 4 == 0)
  ++days;
  return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
  return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime(uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000; // bring to 2000 timestamp from 1970

  ss = t % 60;
  t /= 60;
  mm = t % 60;
  t /= 60;
  hh = t % 24;
  uint16_t days = t / 24;
  uint8_t leap;
  for (yOff = 0;; ++yOff) {
    leap = yOff % 4 == 0;
    if (days < 365 + leap)
      break;
    days -= 365 + leap;
  }
  for (m = 1;; ++m) {
    uint8_t daysPerMonth = daysInMonth[m - 1];
    if (leap && m == 2)
    ++daysPerMonth;
    if (days < daysPerMonth)
      break;
    days -= daysPerMonth;
  }
  d = days + 1;
}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
  if (year >= 2000)
    year -= 2000;
  yOff = year;
  m = month;
  d = day;
  hh = hour;
  mm = min;
  ss = sec;
}

static uint8_t conv2d(const char * p) {
  uint8_t v = 0;
  if ('0' <= * p && * p <= '9')
    v = * p - '0';
  return 10 * v + * ++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
DateTime::DateTime (const char* date, const char* time) {
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  yOff = conv2d(date + 9);
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
  switch (date[0]) {
    case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
    case 'F': m = 2; break;
    case 'A': m = date[2] == 'r' ? 4 : 8; break;
    case 'M': m = date[2] == 'r' ? 3 : 5; break;
    case 'S': m = 9; break;
    case 'O': m = 10; break;
    case 'N': m = 11; break;
    case 'D': m = 12; break;
  }
  d = conv2d(date + 4);
  hh = conv2d(time);
  mm = conv2d(time + 3);
  ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfWeek() const {
  uint16_t day = date2days(yOff, m, d);
  return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000; // seconds from 1970 to 2000

  return t;
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin(uint8_t val) {
  return val - 6 * (val >> 4);
}
static uint8_t bin2bcd(uint8_t val) {
  return val + 6 * (val / 10);
}

uint8_t RTC_DS1307::begin(void) {
  return 1;
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire.read();
  return !(ss >> 7);
}

void RTC_DS1307::adjust(const DateTime & dt) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0);
  Wire.write(bin2bcd(dt.second()));
  Wire.write(bin2bcd(dt.minute()));
  Wire.write(bin2bcd(dt.hour()));
  Wire.write(bin2bcd(0));
  Wire.write(bin2bcd(dt.day()));
  Wire.write(bin2bcd(dt.month()));
  Wire.write(bin2bcd(dt.year() - 2000));
  Wire.write(0);
  Wire.endTransmission();
}

DateTime RTC_DS1307::now() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire.read() & 0x7F);
  uint8_t mm = bcd2bin(Wire.read());
  uint8_t hh = bcd2bin(Wire.read());
  Wire.read();
  uint8_t d = bcd2bin(Wire.read());
  uint8_t m = bcd2bin(Wire.read());
  uint16_t y = bcd2bin(Wire.read()) + 2000;

  return DateTime(y, m, d, hh, mm, ss);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime & dt) {
  offset = dt.unixtime() - millis() / 1000;
}

DateTime RTC_Millis::now() {
  return (uint32_t)(offset + millis() / 1000);
}

////////////////////////////////////////////////////////////////////////////////


/* =========================== APPLICATION.cpp =============================== */

// Date and time functions using a DS1307 RTC connected via I2C
//
// WIRE IT UP!
//
// DS1307               SPARK CORE
//--------------------------------------------------------------------
// VCC                - Vin (5V only, does not work on 3.3)
// Serial Clock (SCL) - D0 (needs 2.2k to 10k pull up resistor to Vin)
// Serial Data  (SDA) - D1 (needs 2.2k to 10k pull up resistor to Vin)
// Ground             - GND
//--------------------------------------------------------------------

RTC_DS1307 rtc;

void setup() {
  Serial1.begin(57600);
  Wire.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
    Serial1.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // ...however it doesn't work in the Spark IDE
    // rtc.adjust(DateTime(__DATE__, __TIME__));
    //
    // Try these methods instead:
    //rtc.adjust(DateTime("Jan 12 2014", "11:26:30")); // date, 24 hour time string
    //rtc.adjust(DateTime(1234567890)); // unix time
    rtc.adjust(DateTime(2014, 1, 31, 23, 59, 59)); // year, month, day, hour, min, sec
  }
}

void loop() {
  DateTime now = rtc.now();

  Serial1.print(now.year(), DEC);
  Serial1.print('/');
  Serial1.print(now.month(), DEC);
  Serial1.print('/');
  Serial1.print(now.day(), DEC);
  Serial1.print(' ');
  Serial1.print(now.hour(), DEC);
  Serial1.print(':');
  Serial1.print(now.minute(), DEC);
  Serial1.print(':');
  Serial1.print(now.second(), DEC);
  Serial1.println();

  Serial1.print(" since midnight 1/1/1970 = ");
  Serial1.print(now.unixtime());
  Serial1.print("s = ");
  Serial1.print(now.unixtime() / 86400L);
  Serial1.println("d");

  // calculate a date which is 7 days and 30 seconds into the future
  DateTime future(now.unixtime() + 7 * 86400L + 30);

  Serial.print(" now + 7d + 30s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();

  Serial.println();
  delay(3000);
}