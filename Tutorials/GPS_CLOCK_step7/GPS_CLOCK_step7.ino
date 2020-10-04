/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 7: THE TIME LIBRARY)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 7 of building a GPS-based clock with TFT display.
                This sketch tests uses the GPS data to set the system
                time using the "TimeLib" library.

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS.h>                               // https://github.com/mikalhart/TinyGPS

TFT_eSPI tft = TFT_eSPI();                         // display object  
TinyGPS gps;                                       // gps object

void syncWithGPS() {                               // set Arduino time from GPS
  byte h,m,s,f,mo,d; int y;                        // hour,min,sec,frac,month,day,year
  long unsigned age;                               // age of data, in milliseconds
  gps.crack_datetime(&y,&mo,&d,&h,&m,&s,&f,&age);  // get time data from GPS
  if (age<1000) setTime(h,m,s,d,mo,y);             // if <1 sec old, set system time
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
    if (gps.encode(c)) {
       syncWithGPS();                                 // if input is complete, use it
       displayTime();
    }
  }
}
