/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 11: LOCAL TIME)
      Author:   Bruce E. Hall, w8bh.net
        Date:   03 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 11 of building a GPS-based clock with TFT display.
                This sketch adds local time display, using the Arduino
                Timezone Library by Jack Christensen

                Modify the two TimeChangeRule lines near top of the sketch
                according to your local time zone and daylight savings.

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus
#include <Timezone.h>                              // https://github.com/JChristensen/Timezone

#define TITLE           "GPS TIME"                 // shown at top of display
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin
#define USE_12HR_FORMAT       true                 // preferred format for local time display
#define TIMECOLOR         TFT_CYAN                 // color of 7-segment time display
#define DATECOLOR       TFT_YELLOW                 // color of displayed month & day
#define LABEL_FGCOLOR   TFT_YELLOW
#define LABEL_BGCOLOR     TFT_BLUE
                                                   
TimeChangeRule myDST                               // Local Timezone setup.  Mine is EST.
  = {"EDT", Second, Sun, Mar, 2, -240};            // Set Daylight time here.  UTC-4hrs
TimeChangeRule mySTD                               // For ex: "First Sunday in Nov at 02:00"
  = {"EST", First, Sun, Nov, 2, -300};             // Set Standard time here.  UTC-5hrs
Timezone myTZ(myDST, mySTD);

TFT_eSPI tft = TFT_eSPI();                         // display object  
TinyGPSPlus gps;                                   // gps object
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
time_t t,lt        = 0;                            // current UTC,local time


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

void displayTime(time_t t) {
  int x=50, y=65, f=7;                             // screen position & font
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  int h=hour(t); int m=minute(t); int s=second(t); // get hours, minutes, and seconds
  if (USE_12HR_FORMAT) {                           // adjust hours for 12 vs 24hr format:
    if (h==0) h=12;                                // 00:00 becomes 12:00
    if (h>12) h-=12;                               // 13:00 becomes 01:00
  }
  if (h<10) x+= tft.drawChar('0',x,y,f);           // leading zero for hours
  x+= tft.drawNumber(h,x,y,f);                     // hours
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  if (m<10) x+= tft.drawChar('0',x,y,f);           // leading zero for minutes
  x+= tft.drawNumber(m,x,y,f);                     // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  if (s<10) x+= tft.drawChar('0',x,y,f);           // add leading zero for seconds
  x+= tft.drawNumber(s,x,y,f);                     // show seconds
}

void displayDate(time_t t) {
  int x=50,y=130,f=4;                              // screen position & font
  const char* days[] = {"Sunday","Monday","Tuesday",
    "Wednesday","Thursday","Friday","Saturday"};
  tft.setTextColor(DATECOLOR, TFT_BLACK);
  tft.fillRect(x,y,265,26,TFT_BLACK);              // erase previous date  
  x+=tft.drawString(days[weekday(t)-1],x,y,f);     // show day of week
  x+=tft.drawString(", ",x,y,f);                   // and     
  x+=tft.drawNumber(month(t),x,y,f);               // show date as month/day/year
  x+=tft.drawChar('/',x,y,f);
  x+=tft.drawNumber(day(t),x,y,f);
  x+=tft.drawChar('/',x,y,f);
  x+=tft.drawNumber(year(t),x,y,f);
}

void updateDisplay() {
  time_t utc=now();                                // check current UTC time
  if (t!=utc) {                                    // is it a new second yet?
    time_t local = myTZ.toLocal(utc);              // get local time
    displayTime(local);                            // and display it
    if (day(local)!=day(lt))                       // did date change? 
      displayDate(local);                          // yes, so display it 
    lt=local;                                      // Remember current local time
    t=utc;                                         // Remember current UTC time  
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
  newScreen();                                     // blank screen w/ title & border
  Serial1.begin(9600);                             // set baud rate of incoming data
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  feedGPS();
  syncCheck();
  updateDisplay();
}
