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

#include "pins_arduino.h"
#include "Arduino.h"

typedef struct {
  volatile uint8_t* port;   // AVR PORT
  volatile uint8_t* ddr;    // AVR PIN
  volatile uint8_t* pin;    // AVR DDR
  uint8_t bitmask; // which bit we are using
  uint8_t calibration; // max number when not touching
} pinsetup_t; // return from readCapacitivePinSetup

pinsetup_t* readCapacitivePinSetup(int pinToMeasure);
uint8_t readCapacitivePin(pinsetup_t* pinSetupInfo);
