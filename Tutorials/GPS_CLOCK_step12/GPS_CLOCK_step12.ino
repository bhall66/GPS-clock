/**************************************************************************
       Title:   GPS clock with TFT display  (STEP 12: CLOCK STATUS)
      Author:   Bruce E. Hall, w8bh.net
        Date:   29 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI,TimeLib,TinyGPS,Timezone libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Step 12 of building a GPS-based clock with TFT display.
                This sketch adds a status indicator, so that we know
                how long its been since GPS synchronization

                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS.h>                               // https://github.com/mikalhart/TinyGPS
#include <Timezone.h>                              // https://github.com/JChristensen/Timezone

#define TITLE           "GPS TIME"                 // shown at top of display
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin
#define USE_12HR_FORMAT       true                 // preferred format for local time display
#define LEADING_ZERO         false                 // show "01:00" vs " 1:00"

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
TinyGPS gps;                                       // gps object
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
time_t t,lt        = 0;                            // current UTC,local time
time_t lastSync    = 0;                            // UTC time of last GPS sync


void ppsHandler() {                                // 1pps interrupt handler:
  pps=1;                                           // set the flag
}

void syncWithGPS() {                               // set Arduino time from GPS
  byte h,m,s,f,mo,d; int y;                        // hour,min,sec,frac,month,day,year
  long unsigned age;                               // age of data, in milliseconds
  gps.crack_datetime(&y,&mo,&d,&h,&m,&s,&f,&age);  // get time data from GPS
  if (age<1000) {                                  // if data is fresh (<1sec old) 
    setTime(h,m,s,d,mo,y);                         // set system time from GPS data
    adjustTime(1);                                 // and adjust forward 1 second
    lastSync = now();                              // remember time of this sync
  }
}

void syncCheck() {
  if (pps) syncWithGPS();                          // is it time to sync with GPS?
  pps=0;                                           // reset flag, regardless
}


void showSatellites() {
  int x=200,y=200,w=50,h=28,ft=4;                  // screen position and size
  unsigned long age;                               // age of gps data, in mS
  int sats = 0;                                    // number of satellites
  gps.get_datetime(NULL,NULL,&age);                // check age if GPS data in mS
  if (age<10000) sats=gps.satellites();            // if <10s old, get sat count
  tft.setTextColor(TFT_YELLOW);                    
  tft.fillRect(x,y,w,h,TFT_BLACK);                 // erase previous count
  tft.drawNumber(sats,x,y,ft);                     // show latest satellite count
}

void showClockStatus () {
  int color,x=20,y=200,w=80,h=20,ft=2;             // screen position and size
  char st[20];                                     // string buffer
  if (!lastSync) return;                           // haven't decoded time yet
  int minPassed = (now()-lastSync)/60;             // how long ago was last decode? 
  itoa(minPassed,st,10);                           // convert number to a string
  strcat(st," min.");                              // like this: "10 min."
  tft.setTextColor(TFT_BLACK);                    
  if (minPassed<60) color=TFT_GREEN;               // green is < 1 hr old
  else if (minPassed<1440) color=TFT_ORANGE;       // orange is 1-24 hr old                 
  else color=TFT_RED;                              // red is >24 hr old
  tft.fillRoundRect(x,y,80,20,5,color);            // show status indicator
  tft.drawString(st,x+10,y+2,ft);                  // and time since last sync
  showSatellites();
}

void displayAMPM (int hr) {
  int x=250,y=90,ft=4;                             // screen position & font
  tft.setTextColor(TIMECOLOR,TFT_BLACK);           // use same color as time
  if (!USE_12HR_FORMAT) 
    tft.fillRect(x,y,50,20,TFT_BLACK);             // 24hr display, so no AM/PM 
  else if (hr<12) 
    tft.drawString("AM",x,y,ft);                   // before noon, so draw AM
  else 
    tft.drawString("PM",x,y,ft);                   // after noon, so draw PM
}

void displayTime(time_t t) {
  int x=20, y=65, f=7;                             // screen position & font
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  int h=hour(t); int m=minute(t); int s=second(t); // get hours, minutes, and seconds
  displayAMPM(h);                                  // display AM/PM, if needed
  if (USE_12HR_FORMAT) {                           // adjust hours for 12 vs 24hr format:
    if (h==0) h=12;                                // 00:00 becomes 12:00
    if (h>12) h-=12;                               // 13:00 becomes 01:00
  }
  if (h<10) {
    if (LEADING_ZERO) 
      x+= tft.drawChar('0',x,y,f);                 // show leading zero for hours
    else {
      tft.setTextColor(TFT_BLACK,TFT_BLACK);       
      x+=tft.drawChar('8',x,y,f);                  // erase old digit
      tft.setTextColor(TIMECOLOR,TFT_BLACK);      
    }
  }
  x+= tft.drawNumber(h,x,y,f);                     // hours
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  if (m<10) x+= tft.drawChar('0',x,y,f);           // leading zero for minutes
  x+= tft.drawNumber(m,x,y,f);                     // show minutes          
  x+= tft.drawChar(':',x,y,f);                     // show ":"
  if (s<10) x+= tft.drawChar('0',x,y,f);           // add leading zero for seconds
  x+= tft.drawNumber(s,x,y,f);                     // show seconds
}

void displayDate(time_t t) {
  int x=20,y=130,f=4;                              // screen position & font
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
    if (minute() != minute(t))                     // are we in a new minute?
      showClockStatus();                           // yes, update clock status
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
  tft.drawString("  Status  ",20,180,2);           // label for clock status
  tft.drawString("  Satellites  ",160,180,2);      // label for segment status
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
