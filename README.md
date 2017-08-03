# DS1307newAlarms
Adding weekday alarms to the DS1307 RTC functionality

This library supports weekday alarms for the DS1307 chip. The alarms that are set are stored into the DS1307 NVRAM memory. This means that if the DS1307 has back up power connected to it the alarms will survice a power outage of the primary power (= usually the MCU power). The library also allows easy check if a weekday alarms needs to go off.


