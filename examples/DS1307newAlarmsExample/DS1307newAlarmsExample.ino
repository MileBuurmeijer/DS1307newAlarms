/**
 * DS1307 Time and Alarm memory handling example using the extended DS1307 library for handling alarms in 
 * the internal free NVRAM of the DS1307
 * 
 * This example sketch shows how to use the extended DS1307 library. This library adds weekday alarms to the DS1307 
 * functionality. These alarms are stored in the DS1307 persistent RAM (NVRAM) so that it survives periods without 
 * power. For that it requires that the DS1307 is powered with an backup battery (e.g. CR2023 coin cell). This sketch 
 * shows:
 * - how to get the current time of the DS1307 chip and if not set yet it will set the time
 * - it shows various example how to set alarms
 * - in the loop part it shows how to check if there is an alarm that neds to go off.
 * - note: weekday alarms can only be set at 5 minute resulution, e.g. 8:10 is OK and 8:09 is not
 *
 * Circuit:
 * - see here for a nice istructable that shows how to connect the DS1307 to an Arduino
 *   http://www.instructables.com/id/Arduino-Real-Time-Clock-DS1307/
 *   
 * Athor:   Milé Buurmeijer
 * Version: 0.6
 * Date:    26/07/2012
 *
 * Version history:
 * 0.1 Initial version that contains alarm structure, alarm retreive functionality and alarm triggering
 * 0.2 Alarm set quick hack
 * 0.3 Setting of alarms debugged
 * 0.4 Removed alarmIsSetAddress * 
 * 0.5 first test of library usage
 * 0.6 alarm clear function corrected
 *
 * Based on test sketch from the "DS1307new" library by Peter Schmelzer and Oliver Kraus (version 1.21)
*/

#define DEBUG 1

#include <Wire.h>       // for some strange reasons, Wire.h must be included here
#include "DS1307new.h"  // extension to new DS1307 Arduino library that includes weekday alarms

void setup() {
  
  #ifdef DEBUG
    // init serial communication
    Serial.begin(9600);
    while(!Serial) {} // for Arduino Leonardo
  #endif
  
  // uncomment line below if you explicitly want to set the time
  //RTC.setRAM(timeIsSetAddress, (uint8_t *)&aspectIsNotSetToken, sizeof(uint8_t)); 

  // check if flagged that time was already set in RTC  
  if (RTC.isTimeSet()) {
    // OK RTCandA was properly set, so let get the time
    RTC.getTime();
  } else {
    // only do this once to set the RTC
    // this routine writes the token to the RTC NV-RAM memory for future reference
    RTC.setDateTimeRTC();
  }
  
  //examples how to set alarms [alarms are defined at at 5 minute resolution]
  RTC.clearAlarmNvramMemory(); // clears the DS1307 internal NVRAM to hold the alarms
  RTC.listNvramMemory(); // print out the NVRAM memory values
  // set alarm Mondays at 5:25, 
  // first parameter is day of the week (sunday = 0, monday = 1, ..., saturday = 6)
  // second parameter is the hour in 24h style
  // last parameter is the minutes in 5 minute resolution, i.e. 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55
  RTC.setAlarm( 1, 5, 25);   
  RTC.listNvramMemory();
  RTC.setAlarm( 2, 16, 35); // set alarm Tuesdays at 16:05
  RTC.listNvramMemory();
  RTC.setAlarm( 3, 12, 10); // aet alarm Wednesday at 12:10
  RTC.listNvramMemory();
  RTC.setAlarm( 4, 11, 15); // set alarm Thursdays at 11:25
  RTC.listNvramMemory();
  RTC.clearAlarm( 4);       // clear alarm on Thursday
  RTC.listNvramMemory();
  RTC.setAlarm ( 6, 20, 0); // set alarm Saturdays at 20:00
  RTC.listNvramMemory();

  #ifdef DEBUG
    Serial.println("DS1307 time and alarm memory module");
    Serial.println("Format is \"hh:mm:ss dd-mm-yyyy DDD\"");
  
    uint8_t CEST = RTC.isCETSummerTime();
    Serial.print("isCETSummerTime=");
    Serial.println(CEST, DEC);    
    Serial.println();
  #endif
}

/**  
 * main loop of the sketch
 */
void loop() {
  RTC.getTime(); // get current time from the DS1307 RTC chip
  RTC.printTime(); // print it to serial
  // check if alarms need to be triggered
  if (RTC.isAlarmTime()) {
    #ifdef DEBUG
      Serial.println("Alarm time has passed!");
    #endif
  }
  delay(10000); // wait 10 seconds
}


