/**************************************************************************
       Title:   GPS Test
      Author:   Bruce E. Hall, w8bh.net
        Date:   20 Apr 2021
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                GPS Module
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI,TinyGPSPlus (install from IDE)
       Legal:   Copyright (c) 2021  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Tests the output of your GPS module
                
                To use, set the baud rate
                and connect the GPS Tx pin to UART input pin (PA10)
                
                Four basic statistics are presented:
                
                1. Chars: the number of characters received over the serial
                connection.  If this number is not increasing over time, then
                no serial data is being received; check your wiring.

                2. Fixes: this represents the number of NMEA sentences that
                have been succesfully decoded and contain a location fix. On
                startup, you might received data for several minutes before a 
                fix can be achieved.  After that, the number should increase
                by 1 or more every 2 seconds.

                3. CS ok: this represents the number of NMEA sentences that
                have been received and found to have a correct checksum value.

                4. Failed: this represents the number of NMEA sentences that
                have been received and found to have an incorrect checksum value.
                It is OK to see a low number here, especially on startup. 
                However, if this number steadily increases, then characters are
                being lost.

                The bottom half of the screen shows the raw data being received,
                in real time.
                

 **************************************************************************/

#include <TFT_eSPI.h>                              // https://github.com/Bodmer/TFT_eSPI
#include <TinyGPS++.h>                             // https://github.com/mikalhart/TinyGPSPlus

#define serial             Serial1                 // hardware UART#1 for GPS data 
#define BAUD_RATE             9600                 // data rate of GPS module


TFT_eSPI tft = TFT_eSPI();                         // display object 
TinyGPSPlus gps;                                   // gps object
int lastUpdate = 0;                                // time of last screen update

void writeChar(char c) {                           // write character to lower screen
  const int left=0, top=160, f=1;                  // top,left position; small font
  static int x=left, y=top;                        // current position
  tft.drawChar(c,x,y,f);                           // draw the character
  x+=8;                                            // advance to next character posn  
  if (c=='\n') {x=left; y+=8;}                     // if end of line, start new line
  if (y>240) {                                     // if bottom of screen
    y=top;                                         // reset to top position 
    tft.fillRect(left,top,320-left,240-top,        // and clear previous characters
      TFT_BLACK);
  }
}

void feedGPS() {                                   // feed GPS into tinyGPS parser
  while (serial.available()) {                     // look for data from GPS module
     char c = serial.read();                       // read in all available chars
     gps.encode(c);                                // feed chars to GPS parser
     writeChar(c);                                 // and show them on screen
  }
}

void updateDisplay() {                             // show basic GPS parser statistics
  const int x=160, f=4;
  if ((millis()-lastUpdate)>2000) {                // update stats every 2 seconds
    lastUpdate = millis();
    tft.drawNumber(gps.charsProcessed(),x,40,f);   // # of character processed
    tft.drawNumber(gps.sentencesWithFix(),x,65,f); // # of NMEA sentences with fix
    tft.drawNumber(gps.passedChecksum(),x,90,f);   // # sentences the passed checksum
    tft.drawNumber(gps.failedChecksum(),x,115,f);  // # sentences that failed checksum
  }
}

void setup() {
  const int x=40, f=4;
  serial.begin(BAUD_RATE);                         // open UART connection to GPS
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);
  tft.drawString("GPS Test",0,0,f);
  tft.drawString("Chars:",x,40,f);
  tft.drawString("Fixes:",x,65,f);
  tft.drawString("CS ok:",x,90,f);
  tft.drawString("Failed: ",x,115,f);
}

void loop() {
  feedGPS();                                       // decode incoming GPS data
  updateDisplay();                                 // show GPS parser statistics                   
}
