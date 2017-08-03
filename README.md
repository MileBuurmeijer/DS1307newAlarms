# DS1307newAlarms
Adding weekday alarms to the DS1307 RTC functionality in this Arduino library

This library supports weekday alarms for the DS1307 chip. The alarms that are set are stored into the DS1307 NVRAM memory. This means that if the DS1307 has back up power connected to it the alarms will survice a power outage of the primary power (= usually the MCU power). The library also allows easy check if a weekday alarms needs to go off.

An nice hardware circuit instructable for connecting a DS1307 real time clock chip to your microcontroller can be found at: 
http://www.instructables.com/id/Arduino-Real-Time-Clock-DS1307/

The library name comes from the original "DS1307new" library (https://github.com/olikraus/ds1307new) with "Alarms" added.
