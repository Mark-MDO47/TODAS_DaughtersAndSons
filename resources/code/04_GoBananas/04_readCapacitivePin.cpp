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

////////////////////////////////////////////////////////////////////////////////////////
// readCapacitivePinSetup
//  Input: Arduino pin number
//  Output: pointer to a struct:
//    LSB 8 bits - port - AVR PORT
//    next 8     - ddr  - AVR PIN
//    next 8     - pin  - AVR DDR
//    next 8     - bitmask - which bit we are using
//
// The core algorithm is from https://playground.arduino.cc/Code/CapacitiveSensor/
//   I tweaked it a little bit...

pinsetup_t* readCapacitivePinSetup(int pinToMeasure) {
  static pinsetup_t pin_setup_return; // readCapacitivePinSetup

  // Here we translate the input pin number from
  //  Arduino pin number to the AVR PORT, PIN, DDR,
  //  and which bit of those registers we care about.
  // We store into our static return area
  pin_setup_return.port = portOutputRegister(digitalPinToPort(pinToMeasure));
  pin_setup_return.ddr = portModeRegister(digitalPinToPort(pinToMeasure));
  pin_setup_return.bitmask = digitalPinToBitMask(pinToMeasure);
  pin_setup_return.pin = portInputRegister(digitalPinToPort(pinToMeasure));
  // calibration is done elsewhere

  // return our static return struct
  return(&pin_setup_return);
} // end readCapacitivePinSetup()

////////////////////////////////////////////////////////////////////////////////////////
// readCapacitivePin
//  Input: A pointer to a pinsetup_t struct
//  Output: A number, from 0 to 17 expressing how much capacitance is on the pin
//  When you touch the pin, or whatever you have attached to it, the number will get larger
//
// The core algorithm is from https://playground.arduino.cc/Code/CapacitiveSensor/

uint8_t readCapacitivePin(pinsetup_t* pinSetupInfo) {
  uint8_t SREG_old; // so we can back up the AVR Status Register
  uint8_t cycles;   // so we can time the rise of the voltage

  // Get fast local variables for AVR PORT, PIN, DDR,
  //  and which bit of those registers we care about.
  volatile uint8_t* port;
  volatile uint8_t* ddr;
  volatile uint8_t* pin;
  byte bitmask;
  port = pinSetupInfo->port;
  ddr = pinSetupInfo->ddr;
  pin = pinSetupInfo->pin;
  bitmask = pinSetupInfo->bitmask;

  // Discharge the pin first by setting it low and output
  *port &= ~(bitmask);
  *ddr  |= bitmask;

  delay(1);

  SREG_old = SREG; //back up the AVR Status Register
  // Prevent the timer IRQ from disturbing our measurement
  noInterrupts();

  // Make the pin an input with the internal pull-up on
  *ddr &= ~(bitmask);
  *port |= bitmask;

  // Now see how long the pin to get pulled up. This manual unrolling of the loop
  // decreases the number of hardware cycles between each read of the pin,
  // thus increasing sensitivity.
  cycles = 17;
  if (*pin & bitmask) { cycles =  0;}
  else if (*pin & bitmask) { cycles =  1;}
  else if (*pin & bitmask) { cycles =  2;}
  else if (*pin & bitmask) { cycles =  3;}
  else if (*pin & bitmask) { cycles =  4;}
  else if (*pin & bitmask) { cycles =  5;}
  else if (*pin & bitmask) { cycles =  6;}
  else if (*pin & bitmask) { cycles =  7;}
  else if (*pin & bitmask) { cycles =  8;}
  else if (*pin & bitmask) { cycles =  9;}
  else if (*pin & bitmask) { cycles = 10;}
  else if (*pin & bitmask) { cycles = 11;}
  else if (*pin & bitmask) { cycles = 12;}
  else if (*pin & bitmask) { cycles = 13;}
  else if (*pin & bitmask) { cycles = 14;}
  else if (*pin & bitmask) { cycles = 15;}
  else if (*pin & bitmask) { cycles = 16;}

  // End of timing-critical section; turn interrupts back on if they were on before, or leave them off if they were off before
  SREG = SREG_old;

  // Discharge the pin again by setting it low and output
  //  It's important to leave the pins low if you want to 
  //  be able to touch more than 1 sensor at a time - if
  //  the sensor is left pulled high, when you touch
  //  two sensors, your body will transfer the charge between
  //  sensors.
  *port &= ~(bitmask);
  *ddr  |= bitmask;

  return cycles;
} // end readCapacitivePin()