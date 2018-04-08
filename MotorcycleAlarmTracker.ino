/*
  Motorcycle burglary alarm

  A project which combines vibration sensor, RTC module, SIM module, GPS module and Arduino as one alarm device. 

  If the motorcycle is moved while this alarm is active it is triggered and send SMS alarm. 

  Otherwise the device wakes up periodically to check SMS messages and act accordingly. Device may be asked to give current position. 

  Connecting modules:
  Pro mini -> Modules
  Pin3 -> GPS-module-RX
  Pin4 -> GPS-module-TX
  Pin5 -> SIM-module-TX
  Pin6 -> SIM-module-RX
  A3 -> Trigger rest pin
  A5 -> RTC-module SCL
  A4 -> RTC-module SDA
  
  Dependency(TinyGPS++ library): http://arduiniana.org/libraries/tinygpsplus/
  Dependency(Adafruit Fona library): https://github.com/adafruit/Adafruit_FONA
  Dependency(DS3231 library): https://github.com/NorthernWidget/DS3231

  created   Aril 2018
  by CheapskateProjects

  ---------------------------
  The MIT License (MIT)

  Copyright (c) 2017 CheapskateProjects

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <DS3231.h>
#include <Wire.h>
#include <Adafruit_FONA.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Pin definitions
#define FONA_RX 11
#define FONA_TX 10
#define FONA_RST 13
#define TRIGGER_RST A3
#define GPS_RX 3
#define GPS_TX 4

// RTC
DS3231 Clock;

// Fona/SIM
char smsBuffer[255];
char senderBuffer[255];
char allowedNumber[14] = "+490XXXXXXXXX"; // Set your own phone number here!
SoftwareSerial FSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *FSerial = &FSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// GPS
TinyGPSPlus gps;
SoftwareSerial gps_ss(GPS_RX, GPS_TX);

/**
 * Reset trigger circuit status to turn the device off
 */
void turnOff()
{
  // Reset alarm if this was time based wakeup
  Clock.checkIfAlarm(1);

  // Reset trigger circuit
  pinMode(TRIGGER_RST, OUTPUT);
  digitalWrite(TRIGGER_RST, HIGH);
}

/**
 * Vibration actions go here. What should the device do when woken up by vibrations
 */
void vibrationTrigger()
{
  fona.sendSMS(allowedNumber, "Alarm!");
  readSMSs();
}

/**
 * Enables the RTC trigger and sets trigger interval according to configuration
 */
void enableRTCTrigger()
{
  Clock.setClockMode(false);
    // 0b1111 // each second
    // 0b1110 // Once per minute (when second matches)
    // 0b1100 // Once per hour (when minute and second matches)
    // 0b1000 // Once per day (when hour, minute and second matches)
    // 0b0000 // Once per month when date, hour, minute and second matches. Once per week if day of the week and A1Dy=true
    
    // Set alarm to happen every hour (change to your wanted interval)
    Clock.setA1Time(1, 1, 1, 0, 0b1100, false, false, false);
    Clock.turnOnAlarm(1);
}

void initializeFona()
{
  // Init module serial
  FSerial->begin(9600);
  if(!fona.begin(*FSerial))
  {
    Serial.println("ERROR: No SIM module!");
    while (1);
  }

  //Give the module some time so that SMS has initialized before trying to read messages
  fona.type();
  delay(30000);
  fona.getNumSMS();
  delay(10000);
  Serial.println("SIM init done");
}

void readSMSs()
{
  Serial.println("SMS");
  int8_t SMSCount = fona.getNumSMS();
  if (SMSCount > 0)
  {
    Serial.println(SMSCount);
    
    uint16_t smslen;
        for (int8_t smsIndex = 1 ; smsIndex <= SMSCount; smsIndex++)
        {
          Serial.print("M:");
          Serial.println(smsIndex);

          // Read message and check error situations
          if (!fona.readSMS(smsIndex, smsBuffer, 250, &smslen))
          {
            Serial.println("Error: Failed");
            continue;
          }
          if (smslen == 0)
          {
            Serial.println("Error: Empty");
            continue;
          }

          // Read sender number so that we can check if it was authorized
          fona.getSMSSender(smsIndex, senderBuffer, 250);

          // Compare against  authorized number
            if(strstr(senderBuffer,allowedNumber))
            {

              // Authorized -> Handle action
              Serial.println("Allowed");
              if(strstr(smsBuffer, "TEST"))
              {
                fona.deleteSMS(smsIndex);
                fona.sendSMS(allowedNumber, "Test OK");
                Serial.println("Sent test");
                break;
              }
              else if(strstr(smsBuffer, "WHERE"))
              {
                sendWhere();
                Serial.println("Sent location as SMS");
              }
              else
              {
                Serial.println("Unknown message type");
                Serial.println(smsBuffer);
              }
            }

          // Message was handled or it was unauthorized -> delete
          fona.deleteSMS(smsIndex);

          // Flush buffers
          while (Serial.available())
          {
            Serial.read();
          }
          while (fona.available())
          {
            fona.read();
          }
    }
  }
}

void sendWhere()
{
  // Start listening location
  gps_ss.listen();
  
  // Wait for next location string
  while(gps_ss.available() <= 0)
  {
    delay(100);
  }

  // Read and send location
  String loc = "Unknown";
  while (gps_ss.available() > 0)
  {
    if (gps.encode(gps_ss.read()))
    {
        if(!gps.location.isValid())
        {
          Serial.println("Not a valid location. Waiting for satelite data.");
          continue;
        }
      
        loc = "";
        loc += String(gps.location.lat(), DEC);
        loc += ",";
        loc += String(gps.location.lng(), DEC);
        Serial.println(loc);
    }
  }
  FSS.listen();
  fona.sendSMS(allowedNumber, loc.c_str());
}

void setup()
{
  // Start the serial port
  Serial.begin(9600);

  // Start the I2C interface
  Wire.begin();
  bool timeTrigger = Clock.checkIfAlarm(1);

  // Starts gps to get the current location
  gps_ss.begin(9600);

  // We always need SMS capabilities
  initializeFona();

  // Alarm is not enabled! Enabling it...
  if(!Clock.checkAlarmEnabled(1))
  {
    enableRTCTrigger();
  }
  
  // Check what woke us up
  if(timeTrigger)
  {
    Serial.println("Time trigger event");
    readSMSs();
  }
  else
  {
    Serial.println("Vibration trigger event");
    vibrationTrigger();
  }

  // Turn off
  delay(30000);
  turnOff();
}

void loop()
{
  // We should never end up here ... 
}
