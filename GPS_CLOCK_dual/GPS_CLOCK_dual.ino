/**************************************************************************
       Title:   GPS clock with dual local/UTC display
      Author:   Bruce E. Hall, w8bh.net
        Date:   04 Ocr 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI,TimeLib,TinyGPS,Timezone libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   GPS digital clock with dual local/UTC display
                Accurate to within a few microseconds!

                Set your local time zone and daylight saving rules at top
                of the sketch under TimeChangeRules: one rule for standard 
                time and one rule for daylight saving time.
                
                Connect GPS "Tx" line to Blue Pill PA10 (Serial RX1)
                Connect GPS "PPS" line to Blue Pill PA11

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS.h>                               // https://github.com/mikalhart/TinyGPS
#include <Timezone.h>                              // https://github.com/JChristensen/Timezone

TimeChangeRule EDT                                 // Local Timezone setup. My zone is EST/EDT.
  = {"EDT", Second, Sun, Mar, 2, -240};            // Set Daylight time here.  EDT = UTC-4hrs
TimeChangeRule EST                                 // For ex: "First Sunday in Nov at 02:00"
  = {"EST", First, Sun, Nov, 2, -300};             // Set Standard time here.  EST = UTC-5hrs
TimeChangeRule *tz;                                // pointer to current time change rule
Timezone myTZ(EDT, EST);                           // create timezone object with rules above

#define GPS_TX                PA10                 // GPS data on hardware serial RX1
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin
#define USING_PPS             true                 // true if GPS_PPS line connected; false otherwise.

#define TITLE            "GPS TIME"                // shown at top of display
#define LOCAL_FORMAT_12HR     true                 // local time format 12hr "11:34" vs 24hr "23:34"
#define UTC_FORMAT_12HR      false                 // UTC time format 12 hr "11:34" vs 24hr "23:34"
#define LEADING_ZERO         false                 // true="01:00", false="1:00"

#define SYNC_INTERVAL          100                 // seconds between GPS synchronizations
#define SYNC_MARGINAL         3600                 // orange status if no sync for 1 hour
#define SYNC_LOST            86400                 // red status if no sync for 1 day   
#define serial Serial1                             // hardware UART#1 for GPS data 

#define TIMECOLOR         TFT_CYAN                 // color of 7-segment time display
#define DATECOLOR       TFT_YELLOW                 // color of displayed month & day
#define LABEL_FGCOLOR   TFT_YELLOW                 // color of label text
#define LABEL_BGCOLOR     TFT_BLUE                 // color of label background


// ============ GLOBAL VARIABLES =====================================================

TinyGPS gps;                                       // gps string-parser object 
TFT_eSPI tft       = TFT_eSPI();                   // display object 
time_t t,oldT      = 0;                            // UTC time, latest & displayed
time_t lt,oldLt    = 0;                            // Local time, latest & displayed
time_t lastSync    = 0;                            // time of last successful sync
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
bool useLocalTime  = false;                        // temp flag used for display updates


// ============ DISPLAY ROUTINES =====================================================

void showAMPM(int hr) {                            // not shown in dual display
}

void showTime(time_t t, bool hr12, int x, int y) {
  const int f=7;                                   // screen font
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  int h=hour(t); int m=minute(t); int s=second(t); // get hours, minutes, and seconds
  showAMPM(h);                                     // display AM/PM, if needed
  if (hr12) {                                      // adjust hours for 12 vs 24hr format:
    if (h==0) h=12;                                // 00:00 becomes 12:00
    if (h>12) h-=12;                               // 13:00 becomes 01:00
  }
  if (h<10) {                                      // is hour a single digit?
    if ((!hr12)||(LEADING_ZERO))                   // 24hr format: always use leading 0
      x+= tft.drawChar('0',x,y,f);                 // show leading zero for hours
    else {
      tft.setTextColor(TFT_BLACK,TFT_BLACK);       // black on black text     
      x+=tft.drawChar('8',x,y,f);                  // will erase the old digit
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

void showDate(time_t t, int x, int y) {
  const int f=4,yspacing=30;                       // screen font, spacing
  const char* months[] = {"JAN","FEB","MAR",
     "APR","MAY","JUN","JUL","AUG","SEP","OCT",
     "NOV","DEC"};
  if (!lastSync) return;                           // if no time yet, forget it
  tft.setTextColor(DATECOLOR, TFT_BLACK);
  int m=month(t), d=day(t);                        // get date components  
  tft.fillRect(x,y,50,60,TFT_BLACK);               // erase previous date       
  tft.drawString(months[m-1],x,y,f);               // show month on top
  y += yspacing;                                   // put day below month
  if (d<10) x+=tft.drawNumber(0,x,y,f);            // draw leading zero for day
  tft.drawNumber(d,x,y,f);                         // draw day
}

void showTimeZone (int x, int y) {
  const int f=4;                                   // text font
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set text colors
  tft.fillRect(x,y,180,28,LABEL_BGCOLOR);          // erase previous TZ
  if (!useLocalTime) 
    tft.drawString("UTC",x,y,f);                   // UTC time
  else if (tz!=NULL)
    tft.drawString(tz->abbrev,x,y,f);              // show local time zone
}

void showTimeDate(time_t t, time_t oldT, bool hr12, int x, int y) {   
  showTime(t,hr12,x,y);                            // display time HH:MM:SS 
  if ((!oldT)||(hour(t)!=hour(oldT)))              // did hour change?
    showTimeZone(x,y-44);                          // update time zone
  if (day(t)!=day(oldT))                           // did date change? 
    showDate(t,x+250,y);                           // update date
}

void showClockStatus() {
  const int x=290,y=1,w=28,h=29,f=2;               // screen position & size
  int color;
  if (second()%10) return;                         // update every 10 seconds
  int syncAge = now()-lastSync;                    // how long has it been since last sync?
  if (syncAge < SYNC_MARGINAL)                     // time is good & in sync
    color = TFT_GREEN;
  else if (syncAge < SYNC_LOST)                    // sync is 1-24 hours old
    color = TFT_ORANGE;
  else color = TFT_RED;                            // time is stale & should not be trusted
  tft.fillRoundRect(x,y,w,h,10,color);             // show clock status as a color
  tft.setTextColor(TFT_BLACK,color);
  tft.drawNumber(satCount(),x+8,y+6,f);            // and number of satellites
}

void newDualScreen() {
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set label colors
  tft.fillScreen(TFT_BLACK);                       // start with empty screen
  tft.fillRoundRect(0,0,319,30,10,LABEL_BGCOLOR);  // title bar for local time
  tft.fillRoundRect(0,126,319,30,10,LABEL_BGCOLOR);// title bar for UTC
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set label colors
  tft.drawRoundRect(0,0,319,110,10,TFT_WHITE);     // draw edge around local time
  tft.drawRoundRect(0,126,319,110,10,TFT_WHITE);   // draw edge around UTC
}


// ============ CLOCK/GPS TIME ROUTINES ===============================================

void setSpecificTime() {                           // for debugging only:
  const int hr=2,mi=59,se=45;                      // choose hour,minute,second here
  const int da=31,mo=5,yr=2020;                    // choose day,month,year here
  setTime(hr,mi,se,da,mo,yr);                      // set arduino time as specified
}

int satCount() {                                   // return # of satellites in view
  unsigned long age;                               // age of gps data, in mS
  gps.get_datetime(NULL,NULL,&age);                // check age if GPS data in mS
  if (age<10000) return gps.satellites();          // if <10s old, get sat count
  else return 0;                                   
}

void syncWithGPS() {                               // set Arduino time from GPS
  byte h,m,s,f,mo,d; int y;                        // hour,min,sec,frac,month,day,year
  long unsigned age;                               // age of data, in milliseconds
  gps.crack_datetime(&y,&mo,&d,&h,&m,&s,&f,&age);  // get time data from GPS
  if (age<1000) {                                  // if data is valid & fresh (<1sec old) 
    setTime(h,m,s,d,mo,y);                         // set system time from GPS data
    adjustTime(1);                                 // and adjust forward 1 second
    lastSync = now();                              // remember time of this sync
  }
}

void syncCheck() {
  if (USING_PPS && !pps) return;                   // only sync at beginning of second
  pps = 0;                                         // reset pps flag
  int timeSinceSync = now()-lastSync;              // how long has it been since last sync?
  if ((timeSinceSync >= SYNC_INTERVAL)             // is it time to sync with GPS?  
  || (!lastSync))  {                               // (or haven't sync'd at all yet)           
    syncWithGPS();                                 // yes: attempt the sync
  }
}

void feedGPS() {                                   // feed GPS into tinyGPS parser
  while (serial.available()) {                     // look for data from GPS module
     char c = serial.read();                       // read in all available chars
     gps.encode(c);                                // and feed each char to GPS parser
  }
}

void ppsHandler() {                                // 1pps interrupt handler:
  pps = 1;                                         // flag that signal was received
}

time_t localTime() {                               // returns local time
  if (!lastSync) return 0;                         // make sure time has been set
  else return myTZ.toLocal(now(),&tz);             // before converting to local
}

void updateDisplay() {
  t = now();                                       // check latest time
  if (t!=oldT) {                                   // are we in a new second yet?
    lt = localTime();                              // keep local time current
    useLocalTime = true;                           // use local timezone
    showTimeDate(lt,oldLt,LOCAL_FORMAT_12HR,10,46);// show new local time
    useLocalTime = false;                          // use UTC timezone
    showTimeDate(t,oldT,UTC_FORMAT_12HR,10,172);   // show new UTC time
     showClockStatus();                            // and clock status
    oldT=t; oldLt=lt;                              // remember currently displayed time
  }
}


// ============ MAIN PROGRAM ===================================================

void setup() {
  serial.begin(9600);                              // open UART connection to GPS
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  newDualScreen();                                 // show title & labels
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  feedGPS();                                       // decode incoming GPS data
  syncCheck();                                     // synchronize time with GPS
  updateDisplay();                                 // keep display current
}
