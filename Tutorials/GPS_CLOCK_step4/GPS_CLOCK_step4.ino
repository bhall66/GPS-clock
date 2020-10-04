/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 4: COUNTING 1PPS SIGNAL)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Build a working GPS-based clock with TFT display
                In this sketch, the GPS 1pps signal is verified
                by using a hardware interrupt.

                See w8bh.net for a detailled, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin

TFT_eSPI tft = TFT_eSPI();                         // display object  
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag

void ppsHandler() {                                // 1pps interrupt handler:
  pps++;                                           // increment pulse count
}

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation 
  tft.fillScreen(TFT_BLACK);                       // start with blank display
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  tft.drawNumber(pps,50,50,7);                      // show pulse count on screen
  delay(100);                                       // wait 0.1s; no need to hurry!
}
