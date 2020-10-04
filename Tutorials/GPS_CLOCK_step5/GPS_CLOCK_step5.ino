/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 5: GPS SERIAL DATA)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 5 of building a GPS-based clock with TFT display
                The sketch tests connectivity with GPS serial data
                by showing the incoming data stream on the display

                Make sure that the GPS Tx line is connected to PA11
                on the Blue Pill.

                See w8bh.net for a detailled, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();                         // display object  

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                       // start with blank display
  Serial1.begin(9600);                             // set baud rate of incoming data
}

void loop() {
  if (Serial1.available()) {                       // if a character is ready to read...
    char c = Serial1.read();                       // get it, and
    tft.print(c);                                  // show it on the display
  }
}
