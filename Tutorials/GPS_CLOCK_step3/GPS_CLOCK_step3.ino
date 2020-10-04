/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 3: HELLO WORLD)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Confirm working CPU and TFT Display
                with a simple "Hello, World!" sketch

                See w8bh.net for a detailled, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>
#define TITLE "Hello, World!"

TFT_eSPI tft = TFT_eSPI();                         // display object            

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLUE);                        // start with empty screen
  tft.setTextColor(TFT_YELLOW);                    // yellow on blue text
  tft.drawString(TITLE,50,50,4);                   // display text
}

void loop() {
}
