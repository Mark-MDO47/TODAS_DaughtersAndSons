
/*
 * Author: https://github.com/Mark-MDO47/
 * 
 * 02_FirstArduinoProject.ino is code for https://github.com/Mark-MDO47/TODAS_DaughtersAndSons 02_FirstArduinoProject
 * 
 * We are using an Arduino Nano with a USB mini-B connector
 *            V3.0 ATmega328P 5V 16M CH340 Compatible to Arduino Nano V3
 *            32Kbyte Flash (program storage), 2Kbyte SRAM, 1Kbyte EEPROM
 *            http://www.mouser.com/pdfdocs/Gravitech_Arduino_Nano3_0.pdf
 *            http://www.pighixxx.com/test/pinouts/boards/nano.pdf
 *
 */

 // connections:
// 
// Nano pin 5V      LEDstick VCC
// Nano pin GND     LEDstick GND
// Nano pin D-7     LEDstick DIN
//
// Nano pin D-3     Button - press to stop action

#include <FastLED.h>

// CUSTOMIZATION CHOICES
long gInterval = 100;           // interval at which to blink (milliseconds)
CRGB gTheColorList[] = { CRGB::Red, CRGB::Green, CRGB::Blue };

// PIN definitions

#define BUTTON_PIN_STOP 3 // press to stop action; release to restart

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 7 // we use this pin for LEDs
// #define CLOCK_PIN 13 // we don't use CLOCK_PIN with this WS2812B LED Strip

// WS2812B and FastLED definitions

// How many leds in your strip?
#define NUM_LEDS 8 // this name is traditional in FastLED code

// brightness
#define BRIGHTMAX 40 // set to 250 for MUCH brighter

// definitions used for routines that display LEDs
static uint8_t gSawtoothPatternBits[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40 };

// this macro allows us to calculate number of elements in an array no matter what size each element is
#define NUMOF(x) (sizeof((x)) / sizeof((*x)))

// the following will adjust automatically to gTheColorList
#define NUMOF_COLOR_LIST NUMOF(gTheColorList)  // if three colors in list then this is 3

// the following will adjust automatically to gSawtoothPatternBits
#define NUMOF_SAWTOOTH_CALLS_THEN_REPEAT NUMOF(gSawtoothPatternBits) // the Sawtooth pattern does this many calls then repeats
static int gPatternsRepeat = NUMOF_SAWTOOTH_CALLS_THEN_REPEAT;

///////////////////////////////////////// GLOBAL VARIABLES //////////////////////////////

// Global variables will change:

// this points to the current color in gTheColorList[]
static int gTheColorIndex = 0;

// Define the RAM storage for the array of leds
CRGB fastled_array[NUM_LEDS];

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// step_color_value() - step through color values
//    updates: gTheColorIndex
//    returns: nothing
//
// NOTE: this routine is designed such that AT ALL TIMES gTheColorIndex is within the valid range.
//   We calculate "idx" and make sure it is valid before storing into gTheColorIndex.

void step_color_value() {
  int idx = gTheColorIndex + 1;
  if (idx >= (NUMOF_COLOR_LIST)) idx = 0;
  gTheColorIndex = idx;
} // end step_color_value()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ptrn_phase() - determine the state of the phase of pattern generation
//    returns: long int with either value >= 0 phase to blink or value < 0 STOP
//

long int ptrn_phase() {
  static long int current_phase = -1;

  if (HIGH == digitalRead(BUTTON_PIN_STOP)) {
    current_phase += 1;
    current_phase %= gPatternsRepeat; // loop through the number of calls before repeat
    if (0 == current_phase) {
      step_color_value();
    }
  } else {
    current_phase = -1; // STOP
  }
  
  return(current_phase);
} // end ptrn_phase()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ptrn_fill(ptrn_leds) - 
//    returns: int with either value HIGH==blinked the LEDs or LOW==did not blink
//
// ptrn_leds   - where to store the pattern

int ptrn_fill(CRGB * ptrn_leds) {
  int did_blink = LOW;
  long int blink_phase = ptrn_phase();
  uint8_t bits = gSawtoothPatternBits[blink_phase];
  long int i; // loop index

  // set RAM for all LEDs off then light up whichever one is our blink_phase (-1 means STOP)
  if (blink_phase >= 0) {
    // motion is still active
    for (i = 0; i < NUM_LEDS; i++) { ptrn_leds[i] = CRGB::Black; }
    for (i = 0; i < NUM_LEDS; i++) {
      if (0 != (0x01 & bits)) {
        ptrn_leds[i] = gTheColorList[gTheColorIndex];
      } // end if this LED is not black
      bits >>= 1;
    } // end for all LEDs/bits
  } // else we are STOP
  did_blink = HIGH;

  return(did_blink);
} // end ptrn_fill()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  while (Serial.available()) Serial.read(); // clear any startup junk from the serial queue

  pinMode(BUTTON_PIN_STOP, INPUT_PULLUP); // digital INPUT_PULLUP means voltage HIGH unless grounded

  // ## Clockless types ##
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(fastled_array, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(BRIGHTMAX); // help keep our power draw through Arduino Nano down

  Serial.println(F("")); // print a blank line in case there is some junk from power-on
  Serial.println(F("TODAS init..."));
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
void loop() { 
  // Generally, you should use "unsigned long" for variables that hold millis() time
  // The value will quickly become too large for an int to store
  static unsigned long previousMillis = 0;        // will preserve last time LED was updated

  // check to see if it's time to "blink" the LED strip; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= gInterval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // generate pattern to display and display it
    ptrn_fill(fastled_array); // fill the pattern into RAM
    FastLED.show(); // show the pattern on LEDs
  } // end if time to display next step in pattern
} // end loop()
