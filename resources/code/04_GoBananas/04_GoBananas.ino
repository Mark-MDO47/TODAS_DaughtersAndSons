// 04_GoBananas
// Author: https://github.com/Mark-MDO47/
//
// The core algorithm is from https://playground.arduino.cc/Code/CapacitiveSensor/
// Here is the code history from that page
// * Original code by Mario Becker, Fraunhofer IGD, 2007 http://www.igd.fhg.de/igd-a4
// * Updated by: Alan Chatham http://unojoy.tumblr.com
// * Updated by Paul Stoffregen: Replaced '328 specific code with portOutputRegister, etc for compatibility with Arduino Mega, Teensy, Sanguino and other boards
// * Gratuitous optimization to improve sensitivity by Casey Rodarmor.
// * Updated by Martin Renold: disable interrupts while measuring. This fixes the occasional too-low results.
// * Updated by InvScribe for Arduino Due.
// * Updated non-Due code by Gabriel Staples (www.ElectricRCAircraftGuy.com), 15 Nov 2014: replaced "interrupts();" with "SREG = SREG_old;" in order to prevent nested interrupts in case you use this function inside an Interrupt Service Routine (ISR).

// I broke the original readCapacitivePin() into two parts
// * readCapacitivePinSetup() - get the bitmasks and do any initialization that can be done
// * readCapacitivePin() - similar to before but different calling sequence
//
// TODO - maybe have it process multiple pins at once?
//   PROs - modified code - everything done in one call - maybe faster
//   CONs - original code - is fast as possible for one pin - maybe faster
//   PLAN - maybe fast enough as is. If not must try it both ways and evaluate.

#include "04_readCapacitivePin.h"

#define DEBUG_SETUP 1 // non-zero to debug size of structs

#define NUM_MEASURE_PINS 4 // number of pins that can do measurement
#define MEASURE_PIN_01 8
#define MEASURE_PIN_02 9
#define MEASURE_PIN_03 10
#define MEASURE_PIN_04 11

#define MEASURE_GUARD 1 // amount greater than calibration to call it a touch

static int pins2measure[NUM_MEASURE_PINS] = { MEASURE_PIN_01, MEASURE_PIN_02, MEASURE_PIN_03, MEASURE_PIN_04 };
static pinsetup_t pin_setup_array[NUM_MEASURE_PINS];


void setup() {
  
  Serial.begin(115200);         // this Serial communication is for general debug; set the USB Serial port to 115,200 baud
  while (!Serial) {
    ; // wait for Serial port to connect. Needed for native USB port only
  }

  pinsetup_t* ptr_pin_setup;
  uint16_t cal_max; // NOTE that these are bigger than uint8_t so can easily compute max+3
  uint16_t cal_return;
  for (uint8_t i = 0; i < NUM_MEASURE_PINS; i++) {
    // get the internal Arduino info to do fast capacitive measurements on our pins
    ptr_pin_setup = readCapacitivePinSetup(pins2measure[i]);
    pin_setup_array[i].port    = ptr_pin_setup->port;
    pin_setup_array[i].ddr     = ptr_pin_setup->ddr;
    pin_setup_array[i].pin     = ptr_pin_setup->pin;
    pin_setup_array[i].bitmask = ptr_pin_setup->bitmask;

    // calibrate this pin when not being touched
    cal_max = 0;
    for (uint8_t j = 0; j < 20; j++) {
      cal_return = readCapacitivePin(&pin_setup_array[i]);
      if (cal_return > cal_max) { cal_max = cal_return; }
    }
    if (255 >= (cal_max+MEASURE_GUARD)) {
      pin_setup_array[i].calibration = cal_max + MEASURE_GUARD;
    } else {
      pin_setup_array[i].calibration = 255; // maximum value
    }
  }

#if DEBUG_SETUP
  Serial.println("DEBUG_SETUP");
  Serial.print("sizeof(pin_setup_array[0]) "); Serial.println(sizeof(pin_setup_array[0]));
  Serial.println(" ");
  for (uint8_t i = 0; i < NUM_MEASURE_PINS; i++) {
    Serial.print("pins2measure[i] "); Serial.println(pins2measure[i]);
    Serial.print("pin_setup_array[i].port "); Serial.println(*pin_setup_array[i].port);
    Serial.print("pin_setup_array[i].ddr "); Serial.println(*pin_setup_array[i].ddr);
    Serial.print("pin_setup_array[i].pin "); Serial.println(*pin_setup_array[i].pin);
    Serial.print("pin_setup_array[i].bitmask 0x"); Serial.println(pin_setup_array[i].bitmask,HEX);
    Serial.print("pin_setup_array[i].calibration "); Serial.println(pin_setup_array[i].calibration);
    Serial.println(" ");
  }
#endif // DEBUG_SETUP
} // end setup()

void loop() {
  uint8_t measured_value;
  static int active_pin = -1; // none touched

  // pay attention to the first pin touched
  for (uint8_t i = 0; i < NUM_MEASURE_PINS; i++) {
    measured_value = readCapacitivePin(&pin_setup_array[i]);
    if (measured_value > pin_setup_array[i].calibration) {
      if (i != active_pin) {
        Serial.print("Start touch pin#"); Serial.print(i); Serial.print(" D"); Serial.println(pins2measure[i]);
        active_pin = i;
      }
      break;
    } else if (i == active_pin) {
      Serial.print("End touch pin#"); Serial.print(i); Serial.print(" D"); Serial.println(pins2measure[i]);
      active_pin = -1; // none touched now
    } // end if
  } // end for loop
} // end loop()
