/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 10: TIME AND DATE)
      Author:   Bruce E. Hall, w8bh.net
        Date:   03 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 10 of building a GPS-based clock with TFT display.
                This sketch adds date display.

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus

#define TITLE     "GPS TIME (UTC)"                 // shown at top of display
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin

#define TIMECOLOR         TFT_CYAN                 // color of 7-segment time display
#define DATECOLOR       TFT_YELLOW                 // color of displayed month & day
#define LABEL_FGCOLOR   TFT_YELLOW
#define LABEL_BGCOLOR     TFT_BLUE

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
  int x=50, y=65, f=7;                             // screen position & font
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  int h=hour(); int m=minute(); int s=second();    // get hours, minutes, and seconds 
  if (h<10) x+= tft.drawChar('0',x,y,f);           // leading zero for hours
  x+= tft.drawNumber(h,x,y,f);                     // hours
  x+= tft.drawChar(':',x,y,f);                     // hour:min separator
  if (m<10) x+= tft.drawChar('0',x,y,f);           // leading zero for minutes
  x+= tft.drawNumber(m,x,y,f);                     // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  if (s<10) x+= tft.drawChar('0',x,y,f);           // add leading zero if needed
  x+= tft.drawNumber(s,x,y,f);                     // show seconds
}

void displayDate() {
  int x=50,y=130,f=4;                              // screen position & font
  const char* days[] = {"Sunday","Monday","Tuesday",
    "Wednesday","Thursday","Friday","Saturday"};
  tft.setTextColor(DATECOLOR, TFT_BLACK);
  tft.fillRect(x,y,265,26,TFT_BLACK);              // erase previous date  
  x+=tft.drawString(days[weekday()-1],x,y,f);      // show day of week
  x+=tft.drawString(", ",x,y,f);                   // and     
  x+=tft.drawNumber(month(),x,y,f);                // show date as month/day/year
  x+=tft.drawChar('/',x,y,f);
  x+=tft.drawNumber(day(),x,y,f);
  x+=tft.drawChar('/',x,y,f);
  x+=tft.drawNumber(year(),x,y,f);
}

void updateDisplay() {
  if (t!=now()) {                                  // is it a new second yet?
    displayTime();                                 // and display it
    if (day(t)!=day())                             // did date change? 
      displayDate();                               // yes, so display it   
    t=now();                                       // Remember current time  
  }
}

void feedGPS() {
  if (Serial1.available()) {                       // if a character is ready to read...
    char c = Serial1.read();                       // get the character
    gps.encode(c);                                 // if input is complete, use it
  }
}

void newScreen() {
  tft.fillScreen(TFT_BLACK);                       // start with empty screen
  tft.fillRoundRect(2,6,316,32,10,LABEL_BGCOLOR);  // put title bar at top
  tft.drawRoundRect(2,6,316,234,10,TFT_WHITE);     // draw edge around screen
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set label colors
  tft.drawCentreString(TITLE,160,12,4);            // show title at top
}

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  newScreen();
  Serial1.begin(9600);                             // set baud rate of incoming data
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  feedGPS();
  syncCheck();
  updateDisplay();
}
