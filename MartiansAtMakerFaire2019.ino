/*
 * Maker Faire 2019 Martians Project, teaches wireless communication
 *
 * Two light creatures, with animated colors point towards each other,
 * and animate when not close to each other.
 *
 * Teaches:
 * Wireless communication
 * GPS location tracking
 * Magnatometer (compass) tracking
 * Data Serialization
 * Real-time and multi-tasking software engineering
 * 8-bit processor environments (Arduino)
 * Battery operated environments
 * LED display technology
 *
 * fcohen@starlingwatch.com
 * May 16, 2019
 *
 * Did you ever see Jim Henson's Martian Muppets on Sesame Street?
 * https://www.youtube.com/watch?v=KTc3PsW5ghQ
 * The Martians inspired me to create a project where two aliens
 * year for each other. When they are within GPS range they
 * look towards each other. When not, they let each other control
 * where they look.
 *
 */

/*
  MartiansAtMakerFaire2019, teaches wireless communication
  Copyright (C) <year>  <name of author>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <ArduinoJson.h>
#include "printf.h"
#include "FastLED.h"    // https://github.com/FastLED/FastLED
#include <math.h>

// Depends on these librararies:
// Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
// Library: Adafruit Sensor and LSM303 compass libraries
// AdruinoJson: to serialize messages between martians

int radioNumber = 0;    // 0 = Transmitter, 1 = Receiver

// ring 0 0-34
// ring 1 35-66
// ring 2 67-95
// ring 3 96-120
// ring 4 121-140

#define ring0 0
#define ring0count 34
#define ring1 35
#define ring1count 31
#define ring2 67
#define ring2count 28
#define ring3 96
#define ring3count 24
#define ring4 121
#define ring4count 19
int rows[] = {ring0, ring1, ring2, ring3, ring4};
int rowscount[] = {ring0count, ring1count, ring2count, ring3count, ring4count};

const float Pi = 3.14159;

static const uint32_t GPSBaud = 9600;

// Light animation show, based on FastLED ColorPalette example
#define UPDATES_PER_SECOND 50
#define LED_PIN     3
#define NUM_LEDS    144
#define BRIGHTNESS  64
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

double magxrec = 0;
double magyrec = 0;
double magzrec = 0;
double lat = 0;
double lng = 0;
double latrec = 0;
double lngrec = 0;

long magxrecvalid = 0;
long magyrecvalid = 0;
long magzrecvalid = 0;
long latvalid = 0;
long lngvalid = 0;
long latrecvalid = 0;
long lngrecvalid = 0;

Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);

TinyGPSPlus gps;

RF24 radio(7, 8); // CE, CSN

const uint64_t pipes[2] = { 0xDBCDABCD71LL, 0xD44d52687CLL };  // Radio pipe addresses for the 2 nodes to communicate.

long messageCounter = 0;
int rrobin = 0;
int rrobin2 = 0;
int rrobin3 = 0;

#define timeoutval 6000

long showtime = millis();
#define showduration 10000
int showstage = 0;
boolean newshow = false;
int showblob = 0;
long showtime2 = millis() + (1000 / UPDATES_PER_SECOND);
uint8_t startIndex = 0;
boolean singleeyeotherflag = false;
boolean singleeyenorthflag = false;

void runLightShow()
{
  // Change to new show

  if ( showtime + showduration < millis() )
  {
    showtime = millis();
    showstage++;
    if ( showstage > 1 ) showstage = 0;

    if ( showstage == 0 )
    {

      // If we have live remote gps then eye towards the other, with a blue background
      if ( lat != 0 && lng != 0 && latrec != 0 && lngrec != 0 )
      {
        for ( int i = 0; i<NUM_LEDS; i++) { leds[ i ] = CRGB::Blue; }
        if ( newshow )
        {
          newshow = false;
        }

        // Yearning: we have recent GPS results, background is blue, eye points towards other's bearing
        SingleEyeTowardsOther( lat, lng, latrec, lngrec, 0 );
        singleeyeotherflag = true;
        singleeyenorthflag = false;
      }
      else
      {
        // If we have live remote compass then point the same heading as the other
        if ( magyrec != 0 && magxrec != 0 )
        {
          if ( newshow )
          {
            newshow = false;
          }
          singleeyenorthflag = false;
          singleeyeotherflag = true;
        }
        else
        {
          // Use the live local compass then point north
          if ( newshow )
          {
            newshow = false;
          }
          // Syncopation (when GPS is invalid, one uses the other's compass heading background is red)
          singleeyenorthflag = true;
          singleeyeotherflag = false;
        }
      }
    }

    // Alternate betweeen RainbowStripeColors and CloudColors
    if ( showstage == 1 )
    {
      if ( showblob == 0 ) { currentPalette = RainbowStripeColors_p; currentBlending = LINEARBLEND; }
      if ( showblob == 1 ) { currentPalette = CloudColors_p; currentBlending = LINEARBLEND; }

      showblob++;
      if ( showblob > 1 ) showblob = 0;
    }

  }

  if (millis() >= showtime2)
  {
    showtime2 = millis() + (1000 / UPDATES_PER_SECOND);

    // Update the light show for rainbows and clouds
    if ( showstage == 0 && singleeyenorthflag )
    {
      sensors_event_t event;
      mag.getEvent(&event);
      SingleEyeNorth( event.magnetic.y, event.magnetic.x, 0 );
    }

    if ( showstage == 0 && singleeyeotherflag )
    {
      // Syncopation (when GPS is invalid, one uses the other's compass heading background is red)
      SingleEyeNorth( magyrec, magxrec, 0 );
    }

    if ( showstage == 1 )
    {
      startIndex = startIndex + 1; /* motion speed */
      FillLEDsFromPaletteColors( startIndex );
    }
    FastLED.show();
  }
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;

    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// Single row eye points north

void SingleEyeNorth( float y, float x, int row)
{
  if ( y == 0 && x == 0 ) return;

  float heading = (atan2(y,x) * 180) / Pi;
  if (heading < 0) heading = 360 + heading;

  for ( int i=0; i<NUM_LEDS; i++ )
  {
    leds[i] = CRGB::DarkRed;
    leds[i].fadeLightBy( 200 );
  }

  int lightnum = (int) heading / ( 360 / rowscount[ row ] );

  leds[ rows[ row ] + lightnum - 1 ] = CRGB::Blue;
  leds[ rows[ row ] + lightnum ] = CRGB::Red;
  leds[ rows[ row ] + lightnum + 1 ] = CRGB::Blue;

  leds[ rows[ row ] + lightnum - 1 ].maximizeBrightness();
  leds[ rows[ row ] + lightnum ].maximizeBrightness();
  leds[ rows[ row ] + lightnum + 1 ].maximizeBrightness();



}

void SingleEyeTowardsOther( double lat1, double lon1, double lat2, double lon2, int row)
{
  if ( lat1 == 0 || lon1 == 0 || lat2 == 0 ||  lon2 == 0  ) return;

  // radians
  float heading = atan2(lat1 - lat2, lon1 - lon2);

  // convert to degrees 0 to 180/-180
  heading *= (180 / 3.14159);

  // convert to 0-360 compass degrees, North = 0
  heading = (450 - (int)heading) % 360;

  for ( int i=0; i<NUM_LEDS; i++ ) leds[i] = CRGB::Black;

  int lightnum = (int) heading / ( 360 / rowscount[ row ] );
  leds[ rows[ row ] + lightnum - 1 ] = CRGB::Blue;
  leds[ rows[ row ] + lightnum ] = CRGB::Red;
  leds[ rows[ row ] + lightnum + 1 ] = CRGB::Blue;
}

void initCompass()
{
  #ifndef ESP8266
    while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
  #endif
  Serial.println("Compass set-up"); Serial.println("");

  /* Enable auto-gain */
  mag.enableAutoRange(true);

  /* Initialise the sensor */
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(10);
}

/* Delay and keeps the GPS feeding data */

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    runLightShow();

    while (Serial3.available()) gps.encode(Serial3.read());

  } while (millis() - start < ms);
}

void timeoutGPSvals()
{
  if ( latrecvalid != 0 )
  {
    if ( millis() - latrecvalid > timeoutval )
    {
      latrecvalid = 0;
      latrec = 0;
    }
  }

  if ( lngrecvalid != 0 )
  {
    if ( millis() - lngrecvalid > timeoutval )
    {
      lngrecvalid = 0;
      lngrec = 0;
    }
  }

  if ( latvalid != 0 )
  {
    if ( millis() - latvalid > timeoutval )
    {
      latvalid = 0;
      lat = 0;
    }
  }

  if ( latvalid != 0 )
  {
    if ( millis() - latvalid > timeoutval )
    {
      latvalid = 0;
      lat = 0;
    }
  }
}

void fadeout()
{
    for( int i = BRIGHTNESS; i > 0; i=i-8)
    {
      FastLED.setBrightness( i );
      FastLED.show();
      delay(100);
    }
}

void fadein()
{
    for( int i = 0; i < BRIGHTNESS; i=i+8)
    {
      FastLED.setBrightness( i );
      FastLED.show();
      delay(100);
    }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
  printf_begin();

  Serial.println( "Martians 2019 by Frank Cohen, fcohen@starlingwatch.com, 2019");

  Serial3.begin(GPSBaud);   // Neo7L GPS

  initCompass();

  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  // Streaking blobs
  currentPalette = RainbowStripeColors_p; currentBlending = LINEARBLEND;

  // Spinning clouds
  //currentPalette = CloudColors_p; currentBlending = LINEARBLEND;

  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(5,15);                 // Smallest time between retries, max no. of retries

  if ( radioNumber == 0 )
  {
    Serial.println("Setup transmitter.");
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
    Serial.println("I am a transmitter.");

  }
  else
  {
    Serial.println("Setup receiver.");
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
    Serial.println("I am a receiver.");
  }

  radio.startListening();
}

void loop()
{
  timeoutGPSvals();

  Serial.print( lat );
  Serial.print( " " );
  Serial.print( lng );
  Serial.print( " " );
  Serial.print( latrec );
  Serial.print( " " );
  Serial.print( lngrec );
  Serial.print( " " );
  Serial.print( magxrec );
  Serial.print( " " );
  Serial.print( magyrec );
  Serial.print( " " );
  Serial.print( magzrec );
  Serial.println( " " );

  if ( radioNumber == 0 )
  {

    // You are the transmitter, send your details

    DynamicJsonDocument doc(30);

    if ( rrobin3 == 0 )
    {
      if ( gps.location.isValid() )
      {
        if ( rrobin2 == 0 )
        {
          doc[ "lat" ] = gps.location.lat();
          lat = gps.location.lat();
          latvalid = millis();
        }
        if ( rrobin2 == 1 )
        {
          doc[ "lng" ] = gps.location.lng();
          lng = gps.location.lng();
          lngvalid = millis();
        }
        rrobin2++;
        if ( rrobin2 > 1 ) rrobin2 = 0;
      }
    }
    else
    {
      sensors_event_t event;
      mag.getEvent(&event);

      if ( rrobin == 0 )
      {
        doc[ "magneticx" ] = event.magnetic.x;
      }
      if ( rrobin == 1 )
      {
        doc[ "magneticy" ] = event.magnetic.y;
      }
      if ( rrobin == 2 )
      {
        doc[ "magneticz" ] = event.magnetic.z;
      }

      rrobin++;
      if ( rrobin >2 ) rrobin = 0;
    }
    rrobin3++;
    if ( rrobin3 > 1 ) rrobin3 = 0;

    char output[30];
    serializeJson(doc, output);

    radio.stopListening();
    radio.write(&output, 30);
    radio.startListening();

    // Wait here until we get a response, or timeout

    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
    {
      smartDelay(10);
      if (millis() - started_waiting_at > 500 )
        timeout = true;
    }

    // Describe the results
    if ( timeout )
    {
      //Serial.println("Timeout waiting for response");
      smartDelay(100);
      return;
    }
    else
    {
      char ackinput[30] = "";

      radio.read(&ackinput, sizeof(ackinput));

      DynamicJsonDocument ack2doc(30);

      DeserializationError error = deserializeJson(ack2doc, ackinput);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println( error.c_str() );
        smartDelay(500);
        return;
      }

      double magx2 = 0;
      double magy2 = 0;
      double magz2 = 0;
      magx2 = ack2doc["magneticx"];
      magy2 = ack2doc["magneticy"];
      magz2 = ack2doc["magneticz"];
      if ( magx2 != 0 )
      {
          magxrec = magx2;
          magxrecvalid = millis();
      }
      if ( magy2 != 0 )
      {
          magyrec = magy2;
          magyrecvalid = millis();
      }
      if ( magz2 != 0 )
      {
          magzrec = magz2;
          magzrecvalid = millis();
      }

      double lat2 = 0;
      double lng2 = 0;
      lat2 = ack2doc["lat"];
      lng2 = ack2doc["lng"];
      if ( lat2 != 0 )
      {
          latrec = lat2;
          latrecvalid = millis();
      }
      if ( lng2 != 0 )
      {
          lngrec = lng2;
          lngrecvalid = millis();
      }
    }
  }
  else
  {
    // You are the receiver

    if ( radio.available() )
    {
      char input[30] = "";
      radio.read(&input, sizeof(input));

      DynamicJsonDocument doc(30);

      DeserializationError error = deserializeJson(doc, input);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        smartDelay(500);
        return;
      }

      double magx2 = 0;
      double magy2 = 0;
      double magz2 = 0;
      magx2 = doc["magneticx"];
      magy2 = doc["magneticy"];
      magz2 = doc["magneticz"];
      if ( magx2 != 0 )
      {
          magxrec = magx2;
          magxrecvalid = millis();
      }
      if ( magy2 != 0 )
      {
          magyrec = magy2;
          magyrecvalid = millis();
      }
      if ( magz2 != 0 )
      {
          magzrec = magz2;
          magzrecvalid = millis();
      }

      double lat2 = 0;
      double lng2 = 0;
      lat2 = doc["lat"];
      lng2 = doc["lng"];
      if ( lat2 != 0 )
      {
          lat = lat2;
          latvalid = millis();
      }
      if ( lng2 != 0 )
      {
          lng = lng2;
          lngvalid = millis();
      }

      // Send the your details to the Transmitter

      DynamicJsonDocument ackdoc(30);

      if ( rrobin3 == 0 )
      {
        if ( gps.location.isValid() )
        {
          if ( rrobin2 == 0 )
          {
            ackdoc[ "lat" ] = gps.location.lat();
            latrec = gps.location.lat();
            latrecvalid = millis();
          }
          if ( rrobin2 == 1 )
          {
            ackdoc[ "lng" ] = gps.location.lng();
            lngrec = gps.location.lat();
            lngrecvalid = millis();
          }

          rrobin2++;
          if ( rrobin2 > 1 ) rrobin2 = 0;
        }
      }
      else
      {
        sensors_event_t event;
        mag.getEvent(&event);

        if ( rrobin == 0 )
        {
          ackdoc[ "magneticx" ] = event.magnetic.x;
        }
        if ( rrobin == 1 )
        {
          ackdoc[ "magneticy" ] = event.magnetic.y;
        }
        if ( rrobin == 2 )
        {
          ackdoc[ "magneticz" ] = event.magnetic.z;
        }

        rrobin++;
        if ( rrobin >2 ) rrobin = 0;
      }
      rrobin3++;
      if ( rrobin3 > 1 ) rrobin3 = 0;

      char ackoutput[30];
      serializeJson(ackdoc, ackoutput);
      radio.stopListening();
      radio.write(&ackoutput, 30);
      radio.startListening();
    }
  }

  smartDelay(1000);
 }
