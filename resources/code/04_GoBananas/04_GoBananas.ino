// 04_GoBananas
// Author: https://github.com/Mark-MDO47/
//

// connections:
//
// Fruits and other capacitive "switches"
//    Nano pin D-8     MEASURE_PIN_01
//    Nano pin D-9     MEASURE_PIN_02
//    Nano pin D-10    MEASURE_PIN_03
//    Nano pin D-11    MEASURE_PIN_04
//
// HC-SR04 Ultrasonic Range Detector
//   Nano pin 5V      SR04 VCC
//   Nano pin GND     SR04 GND
//   Nano pin D-2     SR04 Trig
//   Nano pin D-3     SR04 Echo
//
// YX5200/DFPlayer Sound Player
//   Nano pin D-4     YX5200 TX; Arduino RX
//   Nano pin D-5     YX5200 RX; Arduino TX
//   Nano pin D-6     YX5200 BUSY; HIGH when audio finishes

// The core capacitive sensing algorithm is from https://playground.arduino.cc/Code/CapacitiveSensor/
// I put the original readCapacitivePin()into 04_readCapacitivePin.* and broke it into two parts
//   readCapacitivePinSetup() - get the bitmasks and do any initialization that can be done
//   readCapacitivePin() - similar to before but different calling sequence
// This approach seems to work well for our TODAS course.

// See https://github.com/Mark-MDO47/AudioPlayer-YX5200 for details on using the YX5200/DFPlayer.
//   I usually install a copy of the DFRobot.com DFPlayer code when using it, since I did a lot
//   of debugging to find how to use it on all the variants of the YX5200 I have seen.
//   These DFRobot.com DFPlayer code files are DFRobotDFPlayerMini.*.
//   LICENSE_for_DFRobot_code.txt shows it is OK to do this and describes the legal boundaries
//   for correct usage.


#include "Arduino.h"              // general Arduino definitions plus uint8_t etc.

#include <Ultrasonic.h>           // HC-SR04 Ultrasonic Rane Detector

#include "SoftwareSerial.h"       // to talk to myDFPlayer without using up debug (HW) serial port
#include "DFRobotDFPlayerMini.h"  // to communicate with the YX5200 audio player

#include "04_readCapacitivePin.h" // modified https://playground.arduino.cc/Code/CapacitiveSensor/

////////////////////////////////////////////////////////////////////////////////////////
// definitions for capacitive measurements

#define DEBUG_SETUP 0 // non-zero to debug setup/calibration of capacitive sensors
#define NUM_MEASURE_PINS 4 // number of pins that can do measurement
#define MEASURE_PIN_01 8
#define MEASURE_PIN_02 9
#define MEASURE_PIN_03 10
#define MEASURE_PIN_04 11
#define MEASURE_GUARD 1 // amount greater than calibration to call it a touch
static int pins2measure[NUM_MEASURE_PINS] = { MEASURE_PIN_01, MEASURE_PIN_02, MEASURE_PIN_03, MEASURE_PIN_04 };
static pinsetup_t pin_setup_array[NUM_MEASURE_PINS];

////////////////////////////////////////////////////////////////////////////////////////
// definitions for YX5200/DFPlayer and software serial

#define DPIN_SWSRL_RX    4  // HW - serial in  - talk to DFPlayer audio player (YX5200)
#define DPIN_SWSRL_TX    5  // HW - serial out - talk to DFPlayer audio player (YX5200)
#define DPIN_AUDIO_BUSY  6  // HW - digital input - HIGH when audio finishes
SoftwareSerial mySoftwareSerial(/*rx =*/DPIN_SWSRL_RX, /*tx =*/DPIN_SWSRL_TX); // to talk to YX5200 audio player
              // SoftwareSerial(rxPin,         txPin,       inverse_logic)
DFRobotDFPlayerMini myDFPlayer;                                // to talk to YX5200 audio player
void DFsetup();                                                // how to initialize myDFPlayer
#define SOUNDNUM_INTRO   7 // our introduction
#define SOUNDNUM_CASSINI 8 // Cassini Saturn sound - SPACE!!!
#define SOUND_DEFAULT_VOL     25  // default volume - 25 is pretty good
#define SOUND_BKGRND_VOL      20  // background volume
#define SOUND_ACTIVE_PROTECT 200  // milliseconds to keep SW twiddled sound active after doing myDFPlayer.play(mySound)
uint32_t state_timerForceSoundActv = 0;  // end timer for enforcing SOUND_ACTIVE_PROTECT
uint8_t state_introSoundPlaying = 1; // we start with the intro sound

////////////////////////////////////////////////////////////////////////////////////////
// definitions for HC_SR04 Ultrasonic Range Detector
#define ULTRA_TRIG_PIN 12 // HW - HC-SR04 Trigger digital pin output
#define ULTRA_ECHO_PIN 10 // HW - HC-SR04 Trigger digital echo pin input
#define ULTRA_CM_PER_REGION 9     // SW - HC-SR04 every this many CM is a different pattern
#define ULTRA_IGNORE_INITIAL_CM 3 // SW - HC-SR04 ignore the first 3 CM since valid range starts at 2 CM
// instantiate my HC-SR04 data object
Ultrasonic my_ultra = Ultrasonic(ULTRA_TRIG_PIN, ULTRA_ECHO_PIN); // default timeout is 20 milliseconds

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
