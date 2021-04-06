/**************************************************************************
       Title:   touch_test
      Author:   Bruce E. Hall, w8bh.net
        Date:   06 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Tests the XPT2046 touch controller on your ILI9341 
                display module.
                
                Touch the screen.  If your controller is working,
                A small yellow dot will appear where you touched, 
                and the x,y coordinates will be displayed.

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
TFT_eSPI tft = TFT_eSPI();                         // display object 


bool touched() {                                   // true if user touched screen     
  const int threshold = 500;                       // ignore light touches
  return tft.getTouchRawZ() > threshold;
}

void markLocation(int x, int y) {
  tft.fillRect(200,0,120,20,TFT_BLACK);            // erase previous coordinates
  tft.setCursor(200,0);                            // at screen top,right:
  tft.print(x); tft.print(", ");                   // show the x coordinate
  tft.print(y);                                    // and the y coordinate
  tft.fillCircle(x,y,6,TFT_YELLOW);                // and place a small circle there  
}

void checkForTouch() {
  short unsigned int x, y;
  if (touched()) {                                 // did user touch the display?
    tft.getTouch(&x,&y);                           // get touch coordinates
    if ((x<400)&&(y<400)) {                        // if coordinates are valid,
      markLocation(x,y);                           // show it on the screen
      delay(300);                                  // and wait (touch deboucer)
    }
  }  
}


// ============ MAIN PROGRAM ===================================================

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                       // clear the screen
  tft.setTextFont(4);                              // use font 4 (larger text)
  tft.setCursor(0,0);                              // at screen top-left,
  tft.print("Touch Test");                         // display the title                      
}

void loop() {
  checkForTouch();                                 // test the touch function!                               
}
