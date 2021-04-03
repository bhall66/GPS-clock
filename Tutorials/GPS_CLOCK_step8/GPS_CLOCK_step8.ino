/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 8: PPS SYNCHRONIZATION)
      Author:   Bruce E. Hall, w8bh.net
        Date:   03 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 8 of building a GPS-based clock with TFT display.
                This sketch adds synchronization with the GPS 1PPS
                signal, significantly improving clock accuracy.

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin

TFT_eSPI tft       = TFT_eSPI();                   // display object  
TinyGPSPlus gps;                                   // gps object
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
time_t t           = 0;                            // current system time

void ppsHandler() {                                // 1pps interrupt handler:
  pps=1;                                           // set the flag
}

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
  adjustTime(1);                                   // and adjust forward 1 second
}

void syncCheck() {
  if (pps) syncWithGPS();                          // is it time to sync with GPS?
  pps=0;                                           // reset flag, regardless
}

void displayTime() {
  int x=10, y=50, f=7;                             // screen position & font
  x+= tft.drawNumber(hour(),x,y,f);                // hours
  x+= tft.drawChar(':',x,y,f);                     // hour:min separator
  x+= tft.drawNumber(minute(),x,y,f);              // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  x+= tft.drawNumber(second(),x,y,f);              // show seconds
}

void updateDisplay() {
  if (t!=now()) {                                  // is it a new second yet?
    t=now();                                       // yes, so remember it  
    displayTime();                                 // and display it
  }
}

void feedGPS() {
  if (Serial1.available()) {                       // if a character is ready to read...
    char c = Serial1.read();                       // get the character
    gps.encode(c);                                 // if input is complete, use it
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                       // start with blank display
  Serial1.begin(9600);                             // set baud rate of incoming data
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  feedGPS();
  syncCheck();
  updateDisplay();
}
