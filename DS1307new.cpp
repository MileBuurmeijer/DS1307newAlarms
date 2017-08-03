/**
 * DS1307 Time and alarm memory handling module by Milé Buurmeijer
 * extension to DS1307new Arduino library from Peter Schmelzer and Oliver Kraus
 *
 * Use NV-RAM to store alarms according the following memory structure:
 * - mem pos 0 contains token to reflect status of clock (set or not)
 * - mem pos 1 contains bits for alarm days reflecting days of week with
 *             alarm set (sunday = bit 1, monday = bit 2, ...)
 *             this corresponds to definition of the dow (day of week) variable of the DS1307 RTC class
 *             i.e. Sunday => RTCandA.dow=0; Saturday => RTCandA.dow=6
 * - mem pos 2 to 8 contain time of alarm for that respective day, 
 *             so pos 2 is for first day with bit n = 1 in alarm days bits store in pos 1
 * - time is stored in 5 minute resolution and is between 4:00 and 12:00 (no wake up in the afternoon!?)
 *             = 72 different values; for example 17 = 4:00 + 17 * 0:05 = 5:25;
 *
 * Version: 0.5
 * Date 26/07/2012
 *
 *Version history:
 * 0.1 Initial version that contains alarm structure, alarm retreive functionality and alarm triggering
 * 0.2 Alarm set quick hack
 * 0.3 Setting of alarms debugged
 * 0.4 Removed alarmIsSetAddress
 * 0.5 Refactored into a library class DS1307newAlarms
 *
 * Based on test sketch from DS1307new library by Peter Schmelzer and Oliver Kraus (version 1.21)
 * see original .cpp file header below
 */

// ###########.read##################################################################
// #
// # Scriptname : DS1307new.cpp
// # Author     : Peter Schmelzer
// # Contributor: Oliver Kraus, Milé Buurmeijer
// # contact    : info@schmelle2.de
// # Date       : 2010-11-01
// # Version    : 1.00
// # License    : cc-by-sa-3.0
// #
// # Description:
// # The DS1307new Library
// #
// # Naming Convention:
// #	get...	Get information from the DS1307 hardware
// #	set...	Write information to the DS1307 hardware
// #	fill...		Put some information onto the object, but do not access DS1307 hardware
// #
// # Notes on the date calculation procedures
// #  Written 1996/97 by Oliver Kraus
// #  Published by Heinz Heise Verlag 1997 (c't 15/97)
// #  Completly rewritten and put under GPL 2011 by Oliver Kraus
// #
// #############################################################################
// *********************************************
// INCLUDE
// *********************************************

#include <Wire.h>
#include "DS1307new.h"

// *********************************************
// DEFINE
// *********************************************
#define DS1307_ID 0x68
//#define DEBUG 1

// *********************************************
// Public functions
// *********************************************

/**
 * Clears alarm on given date.
 */
boolean DS1307new::clearAlarm( uint8_t dayOfWeek) {
  // reset alarm address to 0xFF
  uint8_t value = 0xFF;
  setRAM( alarmCodeAddressOffset + dayOfWeek, &value, sizeof(uint8_t));  
  // remove bit from alarm bits, but get latest alarmbits value first
  getRAM( alarmBitsAddress, &value, sizeof(uint8_t));
  uint8_t mask = 1;
  mask = mask << dayOfWeek; // shift mask to dayOfWeek-th bit
  value = value - mask; // only reset dayOfWeek-th bit
  // reset alarm bit of this day of week alarm 
  setRAM( alarmBitsAddress, &value, sizeof(uint8_t));  
}

/**
 * Check if RTC's clock is set
 */
boolean DS1307new::isTimeSet() {
  uint8_t value = 0;
  getRAM( timeIsSetAddress, &value, sizeof(uint8_t));  
  if ( value == aspectIsSetToken) {
    return true;
  }
  return false;
}

/**
 * sets alarm on given date and time. Returns true if succeeeded.
 */
boolean DS1307new::setAlarm( uint8_t dayOfWeek, uint8_t alarmHour, uint8_t alarmMinutes) {
  // calculate alarmCode
  if (alarmHour >= 4 || alarmHour < 21) {
    uint8_t alarmCode = (alarmHour - 4 ) * 12 + alarmMinutes / 5;
    setAlarm (dayOfWeek, alarmCode);
    return true;
  } 
  return false;
}

void DS1307new::setAlarm( uint8_t dayOfWeek, uint8_t alarmCode) {
  // first set the proper alarm bit in NV-RAM
  byte currentAlarmBits = 0;
  getRAM( alarmBitsAddress, &currentAlarmBits, sizeof(uint8_t)); // get current value
  byte alarmBitMask = 1 << dayOfWeek; 
  currentAlarmBits = currentAlarmBits | alarmBitMask;
  setRAM( alarmBitsAddress, &currentAlarmBits, sizeof(uint8_t));
  // then set the corresponding alarmcode in right place in memory
  setRAM( alarmCodeAddressOffset + dayOfWeek, &alarmCode, sizeof(uint8_t));
  
  #ifdef DEBUG
    Serial.print("Alarm set at ");
    Serial.print(4+(alarmCode/12));
    Serial.print(":");
    Serial.print((alarmCode - (alarmCode/12*12))*5, DEC);
    Serial.print(" =alarmCode[");
    Serial.print(alarmCode, DEC);
    Serial.print("], alarmBitMask=");
    Serial.print(alarmBitMask, BIN);
    Serial.print(", currentAlarmBits=");
    Serial.print(currentAlarmBits, BIN);
    Serial.println();
  #endif
}

boolean DS1307new::isAlarmTime() {
  boolean alarmTriggered = false;
  uint8_t mask=1;
  // get current alarm bits
  uint8_t currentAlarmBits = 0; // stores the days of the week that alarm is set
  getRAM( alarmBitsAddress, &currentAlarmBits, sizeof(uint8_t)); // get current value
  // prepare mask
  uint8_t currentDayOfWeek = dow;
  uint8_t currentDayOfWeekMask = mask << currentDayOfWeek;
  // check if alarm bit set for today
  if ((currentAlarmBits & currentDayOfWeekMask) > 0) {
    // OK alarm set for today
    // get todays alarm code (alarm code is nr times 5 minutes past 4:00)
    uint8_t alarmCode = 0;
    getRAM( alarmCodeAddressOffset + currentDayOfWeek, &alarmCode, sizeof(uint8_t));
    // lets calculate the alarm hour: there are 12 times 5 minutes in an hour and divide by twelve rounds to number of hours
    uint8_t alarmHour = (uint8_t) alarmCode / 12; 
    uint8_t alarmMinute = (alarmCode - alarmHour * 12) * 5; // so makes sure we get the remaining 5 minute periods after the alarm hour
    alarmHour += 4; // add 4 hours because the alarm code starts counting at 4 o clock in the morning
    #ifdef DEBUG
      Serial.print("Alarm triggered time is ");
      Serial.print(alarmTriggeredTime, DEC);
      Serial.print(", current time is ");
      print2Decimals( hour);
      Serial.print(":");
      print2Decimals( minute);
      Serial.print(", alarm at ");
      print2Decimals( alarmHour);
      Serial.print(":");
      print2Decimals( alarmMinute);
      Serial.println();
    #endif
    if (alarmTriggeredTime==0 && hour >= alarmHour && minute >= alarmMinute) {
      // trigger the alarm!
      #ifdef DEBUG
	Serial.println("Alarm time has passed!");
      #endif
      alarmTriggeredTime = micros();
      alarmTriggered = true;
    }
    // switch of alarm trigger time at the start of a new day
    if (hour == 0 && minute == 0) {
      alarmTriggeredTime = 0;
    }
  }
  return alarmTriggered;
}

// routine to convert ascii numbers to unsigned integers
// make sure that the input pointer is pointing to right part of string
uint8_t DS1307new::convert2decimal(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
	v = *p - '0';
    return 10 * v + *++p - '0';
}

void DS1307new::setDateTimeRTC() {
  // first stop the clock
  stopClock();
  // use compiler time to set date/time
  setDateTime(__DATE__, __TIME__);
  // set the time
  setTime();
  // start the clcok after setting it
  startClock();
  // store time-is-set token in lowest NV-RAM address (=0x08)
  // note: addressing of NV-RAM is done from virtual address 0 onwards in DS1307_new library so that
  //       there is no risk of overwriting the clock registers between real address 0 and 0x08
  setRAM(timeIsSetAddress, (uint8_t *)&aspectIsSetToken, sizeof(uint16_t));
  #ifdef DEBUG
    Serial.println( "time is set and token registered");
  #endif
}  

// a convenient routine for using "the compiler's time":
// setDateTime(__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
void DS1307new::setDateTime (const char* date, const char* time) {
  uint8_t yOff, m, d, hh, mm, ss;
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  yOff = convert2decimal(date + 9);
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
  d = convert2decimal(date + 4);
  hh = convert2decimal(time);
  mm = convert2decimal(time + 3);
  ss = convert2decimal(time + 6);
  // using DS1307 routines to load date and time into RTC object
  fillByYMD( (uint16_t) (yOff+2000), m, d);
  fillByHMS( hh, mm, ss); 
}

void DS1307new::clearAlarmNvramMemory() {
  //
  setRAM( alarmBitsAddress, &zero, sizeof(uint8_t)); // clear alarm bits
  byte memContent = 0xFF;
  for (int i=alarmCodeAddressOffset; i<=alarmCodeAddressOffset+6; i++) {
    setRAM( i, &memContent, sizeof(byte));
  }
  #ifdef DEBUG
    Serial.println("Alarm memory cleared");
  #endif
}

void DS1307new::listNvramMemory() {
  #ifdef DEBUG
    byte memContent = 0;
    Serial.println("DS1307 memory dump");
    for (int i=0; i<=alarmCodeAddressOffset+6; i++) {
      Serial.print("Mem loc[");
      Serial.print(i,HEX);
      Serial.print("]=");
      getRAM( i, &memContent, sizeof(byte));
      Serial.println(memContent,BIN);
    }
  #endif
}

void DS1307new::print2Decimals( uint8_t number) {
    if (number < 10) {
      Serial.print("0");
    } 
    Serial.print(number, DEC);
}  

void DS1307new::printTime() {
  #ifdef DEBUG
    print2Decimals( hour);
    Serial.print(":");
    print2Decimals( minute);
    Serial.print(":");
    print2Decimals( second);
    Serial.print(" ");
    print2Decimals( day);
    Serial.print("-");
    print2Decimals( month);
    Serial.print("-");
    print2Decimals( year);
    Serial.print(" ");
    switch (dow) {                    // Friendly printout the weekday
      case 1:
        Serial.print("MON");
        break;
      case 2:
        Serial.print("TUE");
        break;
      case 3:
        Serial.print("WED");
        break;
      case 4:
        Serial.print("THU");
        break;
      case 5:
        Serial.print("FRI");
        break;
      case 6:
        Serial.print("SAT");
        break;
      case 7:
        Serial.print("SUN");
        break;
    }
    uint8_t CEST = isCETSummerTime();
    Serial.print(" isCETSummerTime=");
    Serial.print(CEST, DEC);  
    
    getRAM(timeIsSetAddress, (uint8_t *)&aspectIsSetTokenHolder, sizeof(uint8_t));
    if (aspectIsSetTokenHolder == aspectIsSetToken)              // check if the clock was set or not
    {
      Serial.println(" - Clock was set!");
    }
    else
    {
      Serial.println(" - Clock was NOT set!");
    }    
  #endif
}

//original DS1307 library functions

DS1307new::DS1307new()
{
  Wire.begin();
  aspectIsSetToken = 0xa5;          // token used to flag that a certain aspect like time or alarms is set
  aspectIsNotSetToken = 0xff;       // token used to flag that a certain aspect like time or alarms is not set
  timeIsSetAddress = 0;       // first address of NVRAM is for time-is-set token
  alarmBitsAddress = 1;       // third address contains bits that flag the days of the week with an alarm set
  alarmCodeAddressOffset = 2; // offset where alarm codes are stored
  aspectIsSetTokenHolder = 0;       // placeholder for the read token
  zero = 0;                         // zero variable to point to to pass zero to RTC
  alarmTriggeredTime = 0;              // last time the alarm was triggered
}

uint8_t DS1307new::isPresent(void)         // check if the device is present
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission() == 0) return 1;
  return 0;
}

void DS1307new::stopClock(void)         // set the ClockHalt bit high to stop the rtc
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);                      // Register 0x00 holds the oscillator start/stop bit
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ID, 1);
  second = Wire.read() | 0x80;       // save actual seconds and OR sec with bit 7 (sart/stop bit) = clock stopped
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);
  Wire.write((uint8_t)second);                    // write seconds back and stop the clock
  Wire.endTransmission();
}

void DS1307new::startClock(void)        // set the ClockHalt bit low to start the rtc
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);                      // Register 0x00 holds the oscillator start/stop bit
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ID, 1);
  second = Wire.read() & 0x7f;       // save actual seconds and AND sec with bit 7 (sart/stop bit) = clock started
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);
  Wire.write((uint8_t)second);                    // write seconds back and start the clock
  Wire.endTransmission();
}

// Aquire time from the RTC chip in BCD format and convert it to DEC
void DS1307new::getTime(void)
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ID, 7);       // request secs, min, hour, dow, day, month, year
  second = bcd2dec(Wire.read() & 0x7f);// aquire seconds...
  minute = bcd2dec(Wire.read());     // aquire minutes
  hour = bcd2dec(Wire.read());       // aquire hours
  dow = bcd2dec(Wire.read());        // aquire dow (Day Of Week)
  dow--;	//  correction from RTC format (1..7) to lib format (0..6). Useless, because it will be overwritten
  day = bcd2dec(Wire.read());       // aquire day
  month = bcd2dec(Wire.read());      // aquire month
  year = bcd2dec(Wire.read());       // aquire year...
  year = year + 2000;                   // ...and assume that we are in 21st century!
  
  // recalculate all other values
  calculate_ydn();
  calculate_cdn();
  calculate_dow();
  calculate_time2000();
}

// Set time to the RTC chip in BCD format
void DS1307new::setTime(void)
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x00);
  Wire.write(dec2bcd(second) | 0x80);   // set seconds (clock is stopped!)
  Wire.write(dec2bcd(minute));           // set minutes
  Wire.write(dec2bcd(hour) & 0x3f);      // set hours (24h clock!)
  Wire.write(dec2bcd(dow+1));              // set dow (Day Of Week), do conversion from internal to RTC format
  Wire.write(dec2bcd(day));             // set day
  Wire.write(dec2bcd(month));            // set month
  Wire.write(dec2bcd(year-2000));             // set year
  Wire.endTransmission();
}

// Aquire data from the CTRL Register of the DS1307 (0x07)
void DS1307new::getCTRL(void)
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x07);                      // set CTRL Register Address
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ID, 1);       // read only CTRL Register
  while(!Wire.available())
  {
    // waiting
  }
  ctrl = Wire.read();                // ... and store it in ctrl
}

// Set data to CTRL Register of the DS1307 (0x07)
void DS1307new::setCTRL(void)
{
  Wire.beginTransmission(DS1307_ID);
  Wire.write((uint8_t)0x07);                      // set CTRL Register Address
  Wire.write((uint8_t)ctrl);                      // set CTRL Register
  Wire.endTransmission();
}

// Aquire data from RAM of the RTC Chip (max 56 Byte)
void DS1307new::getRAM(uint8_t rtc_addr, uint8_t * rtc_ram, uint8_t rtc_quantity)
{
  Wire.beginTransmission(DS1307_ID);
  rtc_addr &= 63;                       // avoid wrong adressing. Adress 0x08 is now address 0x00...
  rtc_addr += 8;                        // ... and address 0x3f is now 0x38
  Wire.write(rtc_addr);                  // set CTRL Register Address
  if ( Wire.endTransmission() != 0 )
    return;
  Wire.requestFrom(DS1307_ID, (int)rtc_quantity);
  while(!Wire.available())
  {
    // waiting
  }
  for(int i=0; i<rtc_quantity; i++)     // Read x data from given address upwards...
  {
    rtc_ram[i] = Wire.read();        // ... and store it in rtc_ram
  }
}

// Write data into RAM of the RTC Chip
void DS1307new::setRAM(uint8_t rtc_addr, uint8_t * rtc_ram, uint8_t rtc_quantity)
{
  Wire.beginTransmission(DS1307_ID);
  rtc_addr &= 63;                       // avoid wrong adressing. Adress 0x08 is now address 0x00...
  rtc_addr += 8;                        // ... and address 0x3f is now 0x38
  Wire.write(rtc_addr);                  // set RAM start Address 
  for(int i=0; i<rtc_quantity; i++)     // Send x data from given address upwards...
  {
    Wire.write(rtc_ram[i]);              // ... and send it from rtc_ram to the RTC Chip
  }
  Wire.endTransmission();
}

/*
  Variable updates:
    cdn, ydn, year, month, day
*/
void DS1307new::fillByCDN(uint16_t _cdn)
{
  uint16_t y, days_per_year;
  cdn = _cdn;
  y = 2000;
  for(;;)
  {
    days_per_year = 365;
    days_per_year += is_leap_year(y);
    if ( _cdn >= days_per_year )
    {
      _cdn -= days_per_year;
      y++;
    }
    else
      break;
  }
  _cdn++;
  year = y;
  ydn = _cdn;
  calculate_dow();
  calculate_month_by_year_and_ydn();
  calculate_day_by_month_year_and_ydn();
  calculate_time2000();
}

/*
  Variable updates:
    time2000, cdn, ydn, year, month, day, hour, minute, second
*/
void DS1307new::fillByTime2000(uint32_t _time2000)
{
  time2000 = _time2000;
  second = _time2000 % 60;
  _time2000 /= 60;
  minute = _time2000 % 60;
  _time2000 /= 60;
  hour = _time2000 % 24;
  _time2000 /= 24;
  fillByCDN(_time2000);
}

void DS1307new::fillByHMS(uint8_t h, uint8_t m, uint8_t s)
{
  // assign variables
  hour = h;
  minute = m;
  second = s;
  // recalculate seconds since 2000-01-01
  calculate_time2000();
}

void DS1307new::fillByYMD(uint16_t y, uint8_t m, uint8_t d)
{
  // assign variables
  year = y;
  month = m;
  day = d;
  // recalculate depending values
  calculate_ydn();
  calculate_cdn();
  calculate_dow();
  calculate_time2000();
}

// check if current time is central european summer time
uint8_t DS1307new::isCETSummerTime(void)
{
  uint32_t current_time, summer_start, winter_start;
  current_time = time2000;
  
  // calculate start of summer time
  fillByYMD(year, 3, 30);
  fillByHMS(2,0,0);
  fillByCDN(RTC.cdn - RTC.dow);	// sunday before
  summer_start = time2000;
  
  // calculate start of winter
  fillByYMD(year, 10, 31);
  fillByHMS(3,0,0);
  fillByCDN(RTC.cdn - RTC.dow);	// sunday before
  winter_start = time2000;
  
  // restore time
  fillByTime2000(current_time);
  
  // return result
  if ( summer_start <= current_time && current_time < winter_start )
    return 1;
  return 0;  
}


// *********************************************
// Private functions
// *********************************************
// Convert Decimal to Binary Coded Decimal (BCD)
uint8_t DS1307new::dec2bcd(uint8_t num)
{
  return ((num/10 * 16) + (num % 10));
}

// Convert Binary Coded Decimal (BCD) to Decimal
uint8_t DS1307new::bcd2dec(uint8_t num)
{
  return ((num/16 * 10) + (num % 16));
}

/*
  Prototype:
    uint8_t DS1307new::is_leap_year(uint16_t y)
  Description:
    Calculate leap year
  Arguments:
    y   		year, e.g. 2011 for year 2011
  Result:
    0           not a leap year
    1           leap year
*/
uint8_t DS1307new::is_leap_year(uint16_t y)
{
   if ( 
          ((y % 4 == 0) && (y % 100 != 0)) || 
          (y % 400 == 0) 
      )
      return 1;
   return 0;
}

/*
  Prototype:
    void calculate_ydn(void)
  Description:
    Calculate the day number within a year. 1st of Jan has the number 1.
    "Robertson" Algorithm
  Arguments:
    this->year           	year, e.g. 2011 for year 2011
    this->month         	month with 1 = january to 12 = december
    this->day          	day starting with 1
  Result:
    this->ydn		The "day number" within the year: 1 for the 1st of Jan.
*/
void DS1307new::calculate_ydn(void)
{
  uint8_t tmp1; 
  uint16_t tmp2;
  tmp1 = 0;
  if ( month >= 3 )
    tmp1++;
  tmp2 = month;
  tmp2 +=2;
  tmp2 *=611;
  tmp2 /= 20;
  tmp2 += day;
  tmp2 -= 91;
  tmp1 <<=1;
  tmp2 -= tmp1;
  if ( tmp1 != 0 )
    tmp2 += is_leap_year(year);
  ydn = tmp2;
}

/*
  Prototype:
    uint16_t to_century_day_number(uint16_t y, uint16_t ydn)
  Description:
    Calculate days since January, 1st, 2000
  Arguments:
    this->y           year, e.g. 2011 for year 2011
    this->ydn	year day number (1st of Jan has the number 1)
  Result
    this->cdn	days since 2000-01-01 (2000-01-01 has the cdn 0)
*/
void DS1307new::calculate_cdn(void)
{
  uint16_t y = year;
  cdn = ydn;
  cdn--;
  while( y > 2000 )
  {
    y--;
    cdn += 365;
    cdn += is_leap_year(y);
  }
}

/*
  calculate day of week (dow)
  0 = sunday .. 6 = saturday
  Arguments:
    this->cdn	days since 2000-01-01 (2000-01-01 has the cdn 0 and is a saturday)
*/
void DS1307new::calculate_dow(void)
{
  uint16_t tmp;
  tmp = cdn;
  tmp += 6;
  tmp %= 7;
  dow = tmp;
}

/*
  Calculate the seconds after 2000-01-01 00:00. The largest possible
  time is 2136-02-07 06:28:15
  Arguments:
    this->h         hour
    this->m	minutes
    this->s		seconds
    this->cdn	days since 2000-01-01 (2000-01-01 has the cdn 0)
*/
void DS1307new::calculate_time2000(void)
{
  uint32_t t;
  t = cdn;
  t *= 24;
  t += hour;
  t *= 60;
  t += minute;
  t *= 60;
  t += second;
  time2000 = t;
}


uint16_t DS1307new::_corrected_year_day_number(void)
{
   uint8_t a;
   uint16_t corrected_ydn = ydn;
   a = is_leap_year(year);
   if ( corrected_ydn > (uint8_t)(((uint8_t)59)+a) )
   {
      corrected_ydn += 2;
      corrected_ydn -= a;
   }
   corrected_ydn += 91;
   return corrected_ydn;
}

/*
  Variables reads:
    ydn, year 
  Variable updates:
    month
*/
void DS1307new::calculate_month_by_year_and_ydn(void)
{
  uint8_t a;
  uint16_t c_ydn;
  c_ydn = _corrected_year_day_number();
  c_ydn *= 20;
  c_ydn /= 611;
  a = c_ydn;
  a -= 2;
  month = a;  
}

/*
  Variables reads:
    ydn, year, month
  Variable updates:
    day
*/
void DS1307new::calculate_day_by_month_year_and_ydn(void)
{
  uint8_t m;
  uint16_t tmp, c_ydn;
  m = month;
  m += 2;
  c_ydn = _corrected_year_day_number();
  tmp = 611;
  tmp *= m;
  tmp /= 20;
  c_ydn -= tmp;
  day = c_ydn;
}




// *********************************************
// Define user object
// *********************************************
class DS1307new RTC;
