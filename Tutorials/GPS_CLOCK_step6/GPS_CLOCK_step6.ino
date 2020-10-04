/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 6: PARSING THE GPS DATA)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 6 of building a GPS-based clock with TFT display.
                This sketch tests parses the incoming GPS data using
                the "tinyGPS" library, and crudely displays the time

                See w8bh.net for a detailled, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TinyGPS.h>                               // https://github.com/mikalhart/TinyGPS

TFT_eSPI tft = TFT_eSPI();                         // display object  
TinyGPS gps;                                       // gps object

void displayTime() {
  int x=10,y=50,f=7;                               // screen position & font
  byte hr,mn,sec;
  gps.crack_datetime(NULL,NULL,NULL,               // get time, ignoring the date
    &hr,&mn,&sec,NULL,NULL);
  tft.fillRect(x,y,250,60,TFT_BLACK);              // erase old time
  x+= tft.drawNumber(hr,x,y,f);                    // hours
  x+= tft.drawChar(':',x,y,f);                     // hour:min separator
  x+= tft.drawNumber(mn,x,y,f);                    // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  x+= tft.drawNumber(sec,x,y,f);                   // show seconds
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
    if (gps.encode(c))                             // and feed it to the GPS parser.
      displayTime();                               // if NMEA sentence complete, display time
  }
}
