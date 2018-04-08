# MotorcycleAlarmTracker
  A project which combines vibration sensor, RTC module, SIM module, GPS module and Arduino as one alarm device. 

  If the motorcycle is moved while this alarm is active it is triggered and send SMS alarm. 

  Otherwise the device wakes up periodically to check SMS messages and act accordingly. Device may be asked to give current position.

Dependency(TinyGPS++ library): http://arduiniana.org/libraries/tinygpsplus/
Dependency(Adafruit Fona library): https://github.com/adafruit/Adafruit_FONA
Dependency(DS3231 library): https://github.com/NorthernWidget/DS3231
