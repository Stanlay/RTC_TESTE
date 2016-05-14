#define DS1307_ADDRESS 0x68
 
#include "application.h"
#include "DS1307/DS1307.h"

// Date and time functions using a DS1307 RTC connected via I2C
//
// WIRE IT UP!
//
// DS1307               SPARK CORE
//--------------------------------------------------------------------
// VCC                - Vin (5V only, does not work on 3.3)
// Serial Clock (SCL) - D1 (needs 2.2k to 10k pull up resistor to Vin)
// Serial Data  (SDA) - D0 (needs 2.2k to 10k pull up resistor to Vin)
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

  Serial1.print(" now + 7d + 30s: ");
  Serial1.print(future.year(), DEC);
  Serial1.print('/');
  Serial1.print(future.month(), DEC);
  Serial1.print('/');
  Serial1.print(future.day(), DEC);
  Serial1.print(' ');
  Serial1.print(future.hour(), DEC);
  Serial1.print(':');
  Serial1.print(future.minute(), DEC);
  Serial1.print(':');
  Serial1.print(future.second(), DEC);
  Serial1.println();

  Serial1.println();
  delay(3000);
}