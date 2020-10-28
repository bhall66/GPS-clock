/**************************************************************************
       Title:   GPS clock with dual local/UTC display
      Author:   Bruce E. Hall, w8bh.net
        Date:   28 Oct 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                uBlox NEO-M8N GPS Module.
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI, TimeLib, TinyGPSplus, Timezone libraries
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   GPS digital clock with dual local/UTC display
                Accurate to within a few microseconds!

                Set your local time zone and daylight saving rules at top
                of the sketch under TimeChangeRules: one rule for standard 
                time and one rule for daylight saving time.
                
                Connect GPS "Tx" line to Blue Pill PA10 (Serial RX1)
                Connect GPS "PPS" line to Blue Pill PA11
                Set GPS baud rate to match your receiver.
                Choose #chars for GridSquare display (0=don't display)

                This sketch functions the same as GPS_CLOCK_dual.
                It uses the TinyGPSPlus library instead of TinyGPS, so
                that its compatible with newer GPS modules that receive 
                multiple satellite systems (GPS, GLONASS, Galileo, etc).

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TimeLib.h>                               // https://github.com/PaulStoffregen/Time
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus
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
#define BAUD_RATE             9600                 // data rate of GPS module
#define GRID_SQUARE_SIZE         6                 // 0 (none), 4 (EM79), 6 (EM79vr), up to 10 char

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

TinyGPSPlus gps;                                   // gps string-parser object 
TFT_eSPI tft       = TFT_eSPI();                   // display object 
time_t t,oldT      = 0;                            // UTC time, latest & displayed
time_t lt,oldLt    = 0;                            // Local time, latest & displayed
time_t lastSync    = 0;                            // time of last successful sync
volatile byte pps  = 0;                            // GPS one-pulse-per-second flag
bool useLocalTime  = false;                        // temp flag used for display updates
char gridSquare[12]= {0};                          // holds current grid square string


// ============ GRID SQUARE CALCULATOR ================================================

void getGridSquare(char *gs, float lat, float lon, const byte len=10) {
  int lon1,lon2,lon3,lon4,lon5;                    // GridSquare longitude components
  int lat1,lat2,lat3,lat4,lat5;                    // GridSquare latitude components 
  float remainder;                                 // temp holder for residuals

  gs[0] = 0;                                       // if input invalid, return null
  lon += 180;                                      // convert (-180,180) to (0,360)
  lat += 90;                                       // convert (-90,90) to (0,180);
  if ((lon<0)||(lon>360)) return;                  // confirm good lon value
  if ((lat<0)||(lat>180)) return;                  // confirm good lat value
  if (len>10) return;                              // allow output length 0-10 chars
  
  remainder = lon;                                 // Parsing Longitude coordinates:
  lon1 = remainder/20;                             // first: divisions of 20 degrees
  remainder -= lon1*20;                            // remove 1st coord contribution 
  lon2 = remainder/2;                              // second: divisions of 2 degrees
  remainder -= lon2*2;                             // remove 2nd coord contribution
  lon3 = remainder*12;                             // third: divisions of 5 minutes   
  remainder -= lon3/12.0;                          // remove 3nd coord contribution
  lon4 = remainder*120;                            // forth: divisions of 30 seconds
  remainder -= lon4/120.0;                         // remove 4th coord contribution
  lon5 = remainder*2880;                           // fifth: division of 1.25 seconds

  remainder = lat;                                 // Parsing Latitude coordinates:           
  lat1 = remainder/10;                             // first: divisions of 10 degrees
  remainder -= lat1*10;                            // remove 1st coord contribution
  lat2 = remainder;                                // second: divisions of 1 degrees
  remainder -= lat2;                               // remove 2nd coord contribution
  lat3 = remainder*24;                             // third: divisions of 2.5 minutes
  remainder -= lat3/24.0;                          // remove 3rd coord contribution
  lat4 = remainder*240;                            // fourth: divisions of 15 seconds
  remainder -= lat4/240.0;                         // remove 4th coord contribution
  lat5 = remainder*5760;                           // fifth: divisions of 0.625 seconds  

  gs[0] = lon1 + 'A';                              // first coord pair are upper case alpha
  gs[1] = lat1 + 'A';
  gs[2] = lon2 + '0';                              // followed by numbers
  gs[3] = lat2 + '0';
  gs[4] = lon3 + 'a';                              // followed by lower case alpha
  gs[5] = lat3 + 'a';
  gs[6] = lon4 + '0';                              // followed by numbers
  gs[7] = lat4 + '0';
  gs[8] = lon5 + 'A';                              // followed by upper case alpha
  gs[9] = lat5 + 'A';
  gs[len] = 0;                                     // set desired string length (0-10 chars)
}


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

void showGridSquare(int x, int y) {
  const int f=4;                                   // text font
  char gs[12];                                     // buffer for new grid square
  if (!gps.location.isValid()) return;             // leave if no fix
  float lat = gps.location.lat();                  // get latitude
  float lon = gps.location.lng();                  // and longitude
  getGridSquare(gs,lat,lon, GRID_SQUARE_SIZE);     // compute current grid square
  if (strcmp(gs,gridSquare)) {                     // has grid square changed?
    strcpy(gridSquare,gs);                         // update copy
    tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR); // set text colors
    tft.fillRect(x-160,y,160,28,LABEL_BGCOLOR);    // erase previous GS
    tft.drawRightString(gs,x,y,f);                 // show current grid square
  }
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
  showGridSquare(310,128);                         // and grid square
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
  if (gps.satellites.age() < 10000) 
    return gps.satellites.value();                 // if <10s old, get sat count
  else return 0;                                   
}

void syncWithGPS() {                               // set Arduino time from GPS
  int h,m,s,mo,d,y,age;                            // hour,min,sec,month,day,year,age                            
  if (!gps.time.isValid()) return;                 // continue only if valid data present
  age = gps.time.age();                            // age of data, in milliseconds
  h   = gps.time.hour();
  m   = gps.time.minute();
  s   = gps.time.second();
  d   = gps.date.day();
  mo  = gps.date.month();
  y   = gps.date.year();
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
  serial.begin(BAUD_RATE);                         // open UART connection to GPS
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
