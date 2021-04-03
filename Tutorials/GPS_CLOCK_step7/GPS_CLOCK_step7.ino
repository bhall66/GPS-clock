/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 7: THE TIME LIBRARY)
      Author:   Bruce E. Hall, w8bh.net
        Date:   03 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 7 of building a GPS-based clock with TFT display.
                This sketch tests uses the GPS data to set the system
                time using the "TimeLib" library.

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus

TFT_eSPI tft = TFT_eSPI();                         // display object  
TinyGPSPlus gps;                                   // gps object

void syncWithGPS() {                               // set Arduino time from GPS
  if (!gps.time.isValid()) return;                 // continue only if valid data present
  if (gps.time.age()>1000) return;                 // dont use stale data
  int h = gps.time.hour();                         // get hour value
  int m = gps.time.minute();                       // get minute value
  int s = gps.time.second();                       // get second value
  int d = gps.date.day();                          // get day
  int mo= gps.date.month();                        // get month
  int y = gps.date.year();                         // get year
  setTime(h,m,s,d,mo,y);                           // set the system time
}

void displayTime() {
  int x=10, y=50, f=7;                             // screen position & font
  tft.fillRect(x,y,250,60,TFT_BLACK);              // erase old time
  x+= tft.drawNumber(hour(),x,y,f);                // hours
  x+= tft.drawChar(':',x,y,f);                     // hour:min separator
  x+= tft.drawNumber(minute(),x,y,f);              // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  x+= tft.drawNumber(second(),x,y,f);              // show seconds
}

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                       // start with blank display
  Serial1.begin(9600);                             // set baud rate of incoming data
}

void loop() {
  if (Serial1.available()) {                       // if a character is ready to read...
    char c = Serial1.read();                       // get the character
    gps.encode(c);                                 // and feed it to the GPS parser.
    if (gps.time.isUpdated()) {                    // Wait until time has been updated
      syncWithGPS();                               // set Arduino time according to GPS
      displayTime();                               // and display the time
    }
  }
}
