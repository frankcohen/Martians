#Martians

Martians is a fun project teaching how to make wireless animated communication devices.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/dwj0wr4Axek/0.jpg)](https://www.youtube.com/watch?v=dwj0wr4Axek)

fcohen@starlingwatch.com
Martians is a free GPL open source project.
See LICENSE for details.

Martians teaches:
- Wireless communication
- GPS location tracking
- Magnetometer (compass) tracking
- Data Serialization
- Real-time and multi-tasking software engineering
- 8-bit processor environments (Arduino)
- Battery operated and portable environments
- LED display technology

Instructions:
1) Buy the components. See below for a parts list. Cost in 2019 was less than $100 USD.
2) Wire the components. Follow the Wiring Guide.
3) Install the needed libraries to documents/arduino/libraries. See the list below.
4) Create a new Arduino IDE sketch/Project
5) Set the radioNumber value for the first Martian to 0, the second Martian to 1
6) Compile MartiansAtMakerFaire2019.ino in Arduino IDE and upload to the Mega board
7) Repeat 5-6 for the second Martian
8) Have fun with your Martians

Jim Henson’s Muppets work on Sesame Street inspired this project:
https://www.youtube.com/watch?v=KTc3PsW5ghQ

I brought the Martians to the Maker Faire, San Mateo, California. May 17 to 19, 2019.
http://makerfaire.com

Video of Martian's in action:
https://youtu.be/dwj0wr4Axek

Martians is a free GPL open source project. Find all code,
wiring, components, and everything else at:
https://github.com/frankcohen/Martians

See Martians Wiring Guide 2019.pdf for wiring instructions.

Martians uses the following components (parts):
TMRh20/RF24 wireless radios, https://github.com/tmrh20/RF24/
Neo7 GPS board, https://www.ebay.com/i/273760042170?chn=ps
Magnetometer, https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/coding
WS2812b LED strip, http://bit.ly/2W06RPd
Arduino Mega

Software libraries:
TinyGPS+ - object library for working with GPS data
Adafruit_Sensor and Adafruit_LSM303_U to read compass and accelerometer
ArduinoJson - serialize and deserialize data between Martians
FastLED - drives LED display

Plastic housing found at IKEA. SOLVINDEN solar powered table light, $15
https://www.ikea.com/us/en/catalog/products/40422098/

-Frank Cohen, May 16, 2019
