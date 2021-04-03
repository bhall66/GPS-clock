/**************************************************************************
       Title:   GPS Clock with TFT display
      Author:   Bruce E. Hall, w8bh.net
        Date:   03 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Adafruit "Ultimate GPS" module v3
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI,TimeLib,TinyGPSPlus,Timezone libraries (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   GPS Clock, accurate to within a few microseconds!

                Set your local time zone and daylight saving rules at top
                of the sketch under TimeChangeRules: one rule for standard 
                time and one rule for daylight saving time.

                Choose Local/UTC time and 12/24hr format in defines near
                top of the sketch.  These can be toggled during runtime via
                touch presses, if your display module supports touch. 
                
                Connect GPS "Tx" line to Blue Pill PA10 (Serial RX1)
                Connect GPS "PPS" line to Blue Pill PA11

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus
#include <Timezone.h>                              // https://github.com/JChristensen/Timezone

TimeChangeRule EDT                                 // Local Timezone setup. My zone is EST/EDT.
  = {"EDT", Second, Sun, Mar, 2, -240};            // Set Daylight time here.  UTC-4hrs
TimeChangeRule EST                                 // For ex: "First Sunday in Nov at 02:00"
  = {"EST", First, Sun, Nov, 2, -300};             // Set Standard time here.  UTC-5hrs
TimeChangeRule *tz;                                // pointer to current time change rule
Timezone myTZ(EDT, EST);                           // create timezone object with rules above

#define GPS_TX                PA10                 // GPS data on hardware serial RX1
#define GPS_PPS               PA11                 // GPS 1PPS signal to hardware interrupt pin
#define USING_PPS             true                 // true if GPS_PPS line connected; false otherwise.

#define SYNC_MARGINAL         3600                 // orange status if no sync for 3600s = 1 hour
#define SYNC_LOST            86400                 // red status if no sync for 1 day
#define serial             Serial1                 // hardware UART#1 for GPS data 
#define TITLE            "GPS TIME"                // shown at top of display
#define SHOW_LOCAL_TIME       true                 // show local or UTC time?
#define USE_12HR_FORMAT       true                 // use 12hr "11:34" vs 24hr "23:34" format
#define LEADING_ZERO         false                 // show "01:00" vs " 1:00"

#define TIMECOLOR         TFT_CYAN
#define DATECOLOR       TFT_YELLOW
#define ZONECOLOR        TFT_GREEN
#define LABEL_FGCOLOR   TFT_YELLOW
#define LABEL_BGCOLOR     TFT_BLUE


// ============ GLOBAL VARIABLES =====================================================

TFT_eSPI tft = TFT_eSPI();                         // display object 
TinyGPSPlus gps;                                   // gps object
time_t t,lt        = 0;                            // current UTC,local time
time_t lastSync    = 0;                            // UTC time of last GPS sync
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
bool use12hrFormat = USE_12HR_FORMAT;              // 12-hour vs 24-hour format?
bool useLocalTime  = SHOW_LOCAL_TIME;              // display local time or UTC?

typedef struct {
  int x;                                           // x position (left side of rectangle)
  int y;                                           // y position (top of rectangle)
  int w;                                           // width, such that right = x+w
  int h;                                           // height, such that bottom = y+h
} region;

region rPM = {240,70,70,40};                       // AM/PM screen region
region rTZ = {240,30,70,40};                       // Time Zone screen region
region rTime = {20,50,200,140};                    // Time screen region
region rSeg = {160,180,120,40};                    // Segment screen region
region rStatus = {20,180,120,40};                  // Clock status screen region
region rTitle = {5,0,310,25};                      // Title bar screen region


// ============ DISPLAY ROUTINES =====================================================

void showTime(time_t t) {
  int x=20, y=65, f=7;                             // screen position & font
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  int h=hour(t); int m=minute(t); int s=second(t); // get hours, minutes, and seconds
  showAMPM(h);                                     // display AM/PM, if needed
  if (use12hrFormat) {                             // adjust hours for 12 vs 24hr format:
    if (h==0) h=12;                                // 00:00 becomes 12:00
    if (h>12) h-=12;                               // 13:00 becomes 01:00
  }
  if (h<10) {                                      // is hour a single digit?
    if ((!use12hrFormat)||(LEADING_ZERO))          // 24hr format: always use leading 0
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

void showDate(time_t t) {
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

void showAMPM (int hr) {
  int x=250,y=90,ft=4;                             // screen position & font
  tft.setTextColor(TIMECOLOR,TFT_BLACK);           // use same color as time
  if (!use12hrFormat) 
    tft.fillRect(x,y,50,20,TFT_BLACK);             // 24hr display, so no AM/PM 
  else if (hr<12) 
    tft.drawString("AM",x,y,ft);                   // before noon, so draw AM
  else 
    tft.drawString("PM",x,y,ft);                   // after noon, so draw PM
}

void showTimeZone () {
  int x=250,y=50,ft=4;                             // screen position & font
  tft.setTextColor(ZONECOLOR,TFT_BLACK);           // zone has its own color
  if (!useLocalTime) 
    tft.drawString("UTC",x,y,ft);                  // UTC time
  else if (tz!=NULL)
    tft.drawString(tz->abbrev,x,y,ft);             // show local time zone
}

void showTimeDate(time_t t, time_t oldT) {         
  showTime(t);                                     // display time (includes AM/PM)
  if ((!oldT)||(hour(t)!=hour(oldT)))              // did hour change?
    showTimeZone();                                // update time zone
  if (day(t)!=day(oldT))                           // did date change? 
    showDate(t);                                   // update date
}

void showSatellites() {
  int x=200,y=200,w=50,h=28,ft=4;                  // screen position and size
  tft.setTextColor(TFT_YELLOW);                    
  tft.fillRect(x,y,w,h,TFT_BLACK);                 // erase previous count
  tft.drawNumber(satCount(),x,y,ft);               // show latest satellite count
}

void showClockStatus () {
  const int x=20,y=200,w=80,h=20,f=2;              // screen position and size
  int color;  char st[20];                         // string buffer
  if (!lastSync) return;                           // haven't decoded time yet
  int syncAge = now()-lastSync;                    // how long has it been since last sync?
  itoa(syncAge/60,st,10);                          // convert minutes to a string
  strcat(st," min.");                              // like this: "10 min."
  tft.setTextColor(TFT_BLACK);                    
  if (syncAge < SYNC_MARGINAL)                     // time is good & in sync
    color = TFT_GREEN;              
  else if (syncAge < SYNC_LOST)                    // sync is 1-24 hours old
    color = TFT_ORANGE;
  else color=TFT_RED;                              // time sync is stale
  tft.fillRoundRect(x,y,80,20,5,color);            // show status indicator
  tft.drawString(st,x+10,y+2,f);                   // and time since last sync
  showSatellites();
}

void newScreen() {
  tft.fillScreen(TFT_BLACK);                       // start with empty screen
  tft.fillRoundRect(2,6,316,32,10,LABEL_BGCOLOR);  // put title bar at top
  tft.drawRoundRect(2,6,316,234,10,TFT_WHITE);     // draw edge around screen
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set label colors
  tft.drawCentreString(TITLE,160,12,4);            // show title at top
  tft.drawString("  Status  ",20,180,2);           // label for clock status
  tft.drawString("  Satellites  ",160,180,2);      // label for segment status
  tft.setTextColor(DATECOLOR);
  tft.drawString("Invalid Time",20,130,4);         // msg to user before GPS starts
}

// ============ CLOCK & GPS ROUTINES ================================================

time_t localTime() {                
  if (!lastSync) return 0;                         // make sure time has been set
  else return myTZ.toLocal(now(),&tz);             // before converting to local
}

void updateDisplay() {
  time_t utc=now(), local;                         // check current UTC time
  if (t!=utc) {                                    // is it a new second yet?
    local = localTime();                           // get local time
    if (useLocalTime) showTimeDate(local,lt);      // either show local time    
    else showTimeDate(utc,t);                      // or UTC time
    if (minute() != minute(t))                     // are we in a new minute?
      showClockStatus();                           // yes, update clock status 
    lt=local;                                      // Remember current local time
    t=utc;                                         // Remember current UTC time  
  }
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
  lastSync = now();                                // remember time of this sync
}

int satCount() {                                   // return # of satellites in view
  int sats = 0;                                    // number of satellites
  if (gps.satellites.isValid())
    sats = gps.satellites.value();                 // get # of satellites in view
  return sats;                                   
}

void syncCheck() {
  if (pps||(!USING_PPS)) syncWithGPS();            // is it time to sync with GPS?
  pps=0;                                           // reset flag, regardless
}


void feedGPS() {                                   // feed GPS into tinyGPS parser
  while (serial.available()) {                     // look for data from GPS module
     char c = serial.read();                       // read in all available chars
     gps.encode(c);                                // and feed chars to GPS parser
  }
}

void ppsHandler() {                                // 1pps interrupt handler:
  pps = 1;                                         // flag that signal was received
}


// ============ TOUCH ROUTINES ===================================================

bool touched() {                                   // true if user touched screen     
  const int threshold = 500;                       // ignore light touches
  return tft.getTouchRawZ() > threshold;
}

boolean inRegion (region b, int x, int y) {        // true if regsion contains point (x,y)
  if ((x < b.x ) || (x > (b.x + b.w)))             // x coordinate out of bounds? 
    return false;                                  // if so, leave
  if ((y < b.y ) || (y > (b.y + b.h)))             // y coordinate out of bounds?
    return false;                                  // if so, leave 
  return true;                                     // x & y both in bounds 
}

void checkTouchPM(int x, int y) {
  if (inRegion(rPM,x,y)                            // did user touch AM/PM?
  || inRegion(rTime,x,y)) {                        // or did user touch Time?
    use12hrFormat = !use12hrFormat;                // toggle 12/24 hr display
    if (useLocalTime) showTime(lt);                // show local time 
    else showTime(t);                              // or show UTC time 
  }  
}

void checkTouchTZ(int x, int y) {                   
  if (!inRegion(rTZ,x,y)) return;                  // Did user touch TZ area?
  useLocalTime = !useLocalTime;                    // toggle the local/UTC flag
  use12hrFormat = useLocalTime;                    // local time is 12hr; UTC is 24hr
  if (useLocalTime) showTimeDate(lt,0);            // show local time zone
  else showTimeDate(t,0);                          // or show UTC time zone
}

void checkForTouch() {
  uint16_t x, y;
  if (touched()) {                                 // did user touch the display?
    tft.getTouch(&x,&y);                           // get touch coordinates
    checkTouchPM(x,y);                             // act if user touched AM/PM
    checkTouchTZ(x,y);                             // act if user touched timeZone
    delay(300);                                    // touch debouncer
  }  
}


// ============ MAIN PROGRAM ===================================================

void setup() {
  serial.begin(9600);                              // open UART connection to GPS
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  newScreen();                                     // show title & labels
  attachInterrupt(digitalPinToInterrupt(           // enable 1pps GPS time sync
    GPS_PPS), ppsHandler, RISING);
}

void loop() {
  feedGPS();                                       // decode incoming GPS data
  syncCheck();                                     // synchronize time with GPS
  updateDisplay();                                 // keep display current
  checkForTouch();                                 // act on any user touch                                
}
