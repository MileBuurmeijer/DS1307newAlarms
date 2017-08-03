// #############################################################################
// #
// # Scriptname : DS1307new.h
// # Author     : Peter Schmelzer
// # Contributor: Oliver Kraus, Milé Buurmeijer
// # contact    : schmelle2@googlemail.com
// # Date       : 2010-11-01
// # Version    : 0.2
// # License    : cc-by-sa-3.0
// #
// # Description:
// # Headerfile for the DS1307new Library
// # 
// #############################################################################
// *********************************************
// DEFINE
// *********************************************
#ifndef DS1307new_h
#define DS1307new_h

// *********************************************
// INCLUDE
// *********************************************
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
  
// *********************************************
// Library interface description
// *********************************************
class DS1307new
{
  public:
    DS1307new();
    // new additions to library for handling alarms
    boolean isTimeSet();
    boolean clearAlarm( uint8_t dayOfWeek);
    boolean setAlarm( uint8_t dayOfWeek, uint8_t alarmHour, uint8_t alarmMinutes);
    void setAlarm( uint8_t dayOfWeek, uint8_t alarmCode);
    boolean isAlarmTime();
    void setDateTimeRTC();
    void setDateTime (const char* date, const char* time);
    void clearAlarmNvramMemory();
    void listNvramMemory();
    void printTime();
    void print2Decimals( uint8_t number);
    
    // initial DS1307 new library functions
    uint8_t isPresent(void);
    void startClock(void);
    void stopClock(void);
    void setTime(void);
    void getTime(void);
    void getCTRL(void);
    void setCTRL(void);
    void getRAM(uint8_t rtc_addr, uint8_t * rtc_ram, uint8_t rtc_quantity);
    void setRAM(uint8_t rtc_addr, uint8_t * rtc_ram, uint8_t rtc_quantity);
    uint8_t second;
    uint8_t minute;
    uint8_t hour; 
    uint8_t dow;			// day of week, 0 = sunday
    uint8_t day;
    uint8_t month;
    uint16_t year;

    uint8_t ctrl;

    uint16_t ydn;		// day within the year (year day number, starts with 1 = 1. Jan)
    uint16_t cdn;		// days after 2000-01-01 (century day number, starts with 0)
    uint32_t time2000;		// seconds after 2000-01-01 00:00 (max value: 2136-02-07 06:28:15)
  
    void fillByCDN(uint16_t _cdn);
    void fillByTime2000(uint32_t _time2000);
    void fillByHMS(uint8_t h, uint8_t m, uint8_t s);
    void fillByYMD(uint16_t y, uint8_t m, uint8_t d);
    uint8_t isCETSummerTime(void);

  private:
    // new additions to DS1307new library fro alarm handling
    uint8_t aspectIsSetToken;          // token used to flag that a certain aspect like time or alarms is set
    uint8_t aspectIsNotSetToken;       // token used to flag that a certain aspect like time or alarms is not set
    uint8_t timeIsSetAddress;       // first address of NVRAM is for time-is-set token
    uint8_t alarmBitsAddress;       // third address contains bits that flag the days of the week with an alarm set
    uint8_t alarmCodeAddressOffset; // offset where alarm codes are stored
    uint8_t aspectIsSetTokenHolder;       // placeholder for the read token
    uint8_t zero;                         // zero variable to point to to pass zero to RTC
    long alarmTriggeredTime;              // last time the alarm was triggered
    uint8_t convert2decimal(const char* p);
    // existing DS1307new library private parts
    uint8_t is_leap_year(uint16_t y);
    void calculate_ydn(void);			// calculate ydn from year, month & day
    void calculate_cdn(void);			// calculate cdn from year & ydn
    void calculate_dow(void);			// calculate dow from ydn
    void calculate_time2000(void);		// calculate time2000 from cdn, hour, minute & second

    uint16_t _corrected_year_day_number(void);
    void calculate_month_by_year_and_ydn(void);
    void calculate_day_by_month_year_and_ydn(void);

    uint8_t dec2bcd(uint8_t num);
    uint8_t bcd2dec(uint8_t num);
};

extern DS1307new RTC;

#endif
