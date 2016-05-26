
/*
 *  Adalight implementation for the ESP arduino environment.
 *  Adjust Serial speed, number of LEDs, and PIN out.
 *  Use with the NEOPIXELBUS libruary found here....https://github.com/Makuna/NeoPixelBus
 *  Inspiration for this came from
 *  https://github.com/adafruit/Adalight.git
 *
 *  This sketch is designed to recieve LED colour information by serial.
 *  ORDER OF LEDS MUST BE  GREEN - RED - BLUE .  This can be changed in the hyperion set up.
 *
 *
 *
 *  This code can be used however, anyone wants.  Consider it BEERWARE!
 *
 */

#include <Arduino.h>

#include <NeoPixelBus.h>

#define debug false

// this example assumes 4 pixels, making it smaller will cause a failure
#define pixelCount 6
#define pixelPin 2 // make sure to set this to the correct pin
const int SerialSpeed = 115200;

NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(pixelCount, pixelPin);

uint8_t *pixelsPOINT = (uint8_t *)strip.Pixels(); ///  used for direct access to pixelbus buffer...

uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;
uint16_t effectbuf_position = 0;
enum mode { MODE_INITIALISE = 0, MODE_HEADER, MODE_CHECKSUM, MODE_DATA, MODE_SHOW, MODE_FINISH };
mode state = MODE_INITIALISE;
int effect_timeout = 0;
uint8_t prefixcount = 0;
unsigned long ada_sent = 0;
const unsigned long serialTimeout = 15000;
long update_strip_time = 0;


void Adalight_Flash() {

  for (int c = -255; c < 255; c+=5) {
    for (uint16_t i = 0; i < strip.PixelCount(); i++) {
      strip.SetPixelColor(i, RgbColor(255-abs(c), 0, 0));
    }
    strip.Show();
    delay(3);
  }
  for (int c = -255; c < 255; c+=5) {
    for (uint16_t i = 0; i < strip.PixelCount(); i++) {
      strip.SetPixelColor(i, RgbColor(0, 255-abs(c), 0));
    }
    strip.Show();
    delay(3);
  }
  for (int c = -255; c < 255; c+=5) {
    for (uint16_t i = 0; i < strip.PixelCount(); i++) {
      strip.SetPixelColor(i, RgbColor(0, 0, 255-abs(c)));
    }
    strip.Show();
    delay(3);
  }
  for (uint16_t i = 0; i < strip.PixelCount(); i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
}

void Adalight() {

  switch (state) {

  case MODE_INITIALISE:
    Serial.println("Begining of Adalight");
    Adalight_Flash();
    state = MODE_HEADER;

  case MODE_HEADER:
    effectbuf_position = 0; // reset the buffer position for DATA collection...

    if (Serial.available()) { // if there is serial available... process it... could be 1  could be 100....
      if(debug) Serial.println("header");

      for (int i = 0; i < Serial.available(); i++) { // go through every character in serial buffer looking for prefix...

        if (Serial.read() == prefix[prefixcount]) { // if character is found... then look for next...
          prefixcount++;
        } else {
          prefixcount = 0; //  otherwise reset....  ////
        }
        if (prefixcount == 3) {
          effect_timeout = millis(); // generates START TIME.....
          state = MODE_CHECKSUM;     // Move on to next state
          prefixcount = 0; // keeps track of prefixes found... might not be the best way to do this.
          break;
        } // end of if prefix == 3

      } // end of for loop going through serial....
    } else if (!Serial.available() && (ada_sent + 5000) < millis()) {
      Serial.print("Ada\n"); // Send "Magic Word" string to host every 5 seconds...
      ada_sent = millis();
    }

    break;

  case MODE_CHECKSUM:
    if (Serial.available() >= 3) {
      if(debug) Serial.println("checksum");
      hi = Serial.read();
      lo = Serial.read();
      chk = Serial.read();
      if (chk == (hi ^ lo ^ 0x55)) {
        if(debug) Serial.println("data");
        state = MODE_DATA;
      } else {
        if(debug) Serial.println("checksum failed");
        state = MODE_HEADER; // ELSE RESET.......
      }
    }

    if ((effect_timeout + 1000) < millis())
      state = MODE_HEADER; // RESET IF BUFFER NOT FILLED WITHIN 1 SEC.

    break;

  case MODE_DATA:
    while (Serial.available() && effectbuf_position < 3 * strip.PixelCount()) {
      pixelsPOINT[effectbuf_position++] = Serial.read();
    }

    if (effectbuf_position >= 3 * pixelCount) { // goto show when buffer has recieved enough data...
      state = MODE_SHOW;
      break;
    }

    if ((effect_timeout + 1000) < millis())
      state = MODE_HEADER; // RESUME HEADER SEARCH IF BUFFER NOT FILLED WITHIN 1 SEC.

    break;

  case MODE_SHOW:
    strip.Dirty(); // MUST USE if you're using the direct buffer version...
    strip.Show();
    state = MODE_HEADER;
    break;

  case MODE_FINISH:

    // nothing in here...

    break;
  }
}

void setup() {
  // this resets all the neopixels to an off state
  Serial.begin(SerialSpeed);

  strip.Begin();
  strip.Show();
}

void loop() {

  // if (millis() - update_strip_time > 30) {
  //   strip.Show();
  //   update_strip_time = millis();
  // };

  Adalight();
}
