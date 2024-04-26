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

#define DEBUG_CAPACITIVE_SETUP 0 // non-zero to debug setup/calibration of capacitive sensors
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
              // SoftwareSerial(rxPin,                 txPin,       inverse_logic)
DFRobotDFPlayerMini myDFPlayer;                                // to talk to YX5200 audio player
void DFsetup();                                                // how to initialize myDFPlayer
#define SOUNDNUM_INTRO   7 // our introduction sound - optional snippet of "Bananaphone" by Raffi
#define SOUND_DEFAULT_VOL     25  // default volume - 25 is pretty good
#define SOUND_BKGRND_VOL      20  // background volume
#define SOUND_ACTIVE_PROTECT 200  // milliseconds to keep SW twiddled sound active after doing myDFPlayer.play(mySound)
uint32_t state_timerForceSoundActv = 0;  // end timer for enforcing SOUND_ACTIVE_PROTECT
uint8_t state_introSoundPlaying = 1; // we start with the intro sound
// our current sound pattern number
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gPrevPattern = 254; // previous pattern number (not present)
uint8_t gPatternNumberChanged = 0; // non-zero if need to change pattern number
#define DFCHANGEVOLUME 0 // zero does not change sound
// #define DFPRINTDETAIL 1 // if need detailed status from myDFPlayer (YX5200 communications)
#define DFPRINTDETAIL 0  // will not print detailed status from myDFPlayer
#if DFPRINTDETAIL // routine to do detailed debugging
  void DFprintDetail(uint8_t type, int value); // definition of call
#else  // no DFPRINTDETAIL
  #define DFprintDetail(type, value) // nothing at all
#endif // #if DFPRINTDETAIL

////////////////////////////////////////////////////////////////////////////////////////
// definitions for HC_SR04 Ultrasonic Range Detector
#define ULTRA_TRIG_PIN 12 // HW - HC-SR04 Trigger digital pin output
#define ULTRA_ECHO_PIN 10 // HW - HC-SR04 Trigger digital echo pin input
#define ULTRA_CM_PER_REGION 9     // SW - HC-SR04 every this many CM is a different pattern
#define ULTRA_IGNORE_INITIAL_CM 3 // SW - HC-SR04 ignore the first 3 CM since valid range starts at 2 CM
int gUltraDistance = 0; // latest measured distance in centimeters
// instantiate my HC-SR04 data object
Ultrasonic my_ultra = Ultrasonic(ULTRA_TRIG_PIN, ULTRA_ECHO_PIN); // default timeout is 20 milliseconds


/////////////////////////////////////////////////////////////////////////////////////////////////////////
#if DFPRINTDETAIL
void DFprintDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!")); break;
    case WrongStack:
      Serial.println(F("Stack Wrong!")); break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!")); break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!")); break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!")); break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!"); break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!"); break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:")); Serial.print(value); Serial.println(F(" Play Finished!"));  break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      } // end switch (value)
      break;
    default:
      break;
  }  // end switch (type)
} // end DFprintDetail()
#endif // DFPRINTDETAIL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFstartSound(tmpSoundNum, tmpVolume) - start tmpSoundNum if it is valid
//
// tmpSoundNum is the sound file number.
//
// Had lots of trouble with reliable operation using playMp3Folder. Came to conclusion
//    that it is best to use the most primitive of YX5200 commands.
// Also saw strong correlation of using YX5200 ACK and having even more unreliable
//    operation, so turned that off in DFinit.
// The code checking DPIN_AUDIO_BUSY still needs the delay but 200 millisec is probably overkill.
// There is code checking myDFPlayer.available() that can maybe also be removed now
//    that the dubugging for above is finished. Now that I am using myDFPlayer.play(),
//    it only seems to trigger when I interrupt a playing sound by starting another.
//    It is sort of interesting but not needed.
//
void  DFstartSound(uint16_t tmpSoundNum, uint16_t tmpVolume) {
  uint16_t idx;
  bool prevHI;
  uint16_t mySound = tmpSoundNum;
  static uint8_t init_minmax = 2;
  static uint32_t prev_msec;
  static uint32_t max_msec = 0;
  static uint32_t min_msec = 999999;
  uint32_t diff_msec = 0;
  uint32_t now_msec = millis();


#if DFCHANGEVOLUME
  myDFPlayer.volume(tmpVolume);  // Set volume value. From 0 to 30 - FIXME 25 is good
#if DFPRINTDETAIL
  if (myDFPlayer.available()) {
    Serial.print(F(" DFstartSound ln ")); Serial.print((uint16_t) __LINE__); Serial.println(F(" myDFPlayer problem after volume"));
    DFprintDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
#endif // DFPRINTDETAIL
#endif // DFCHANGEVOLUME

  myDFPlayer.play(mySound); //play specific mp3 in SD: root directory ###.mp3; number played is physical copy order; first one copied is 1
  // Serial.print(F("DEBUG DFstartSound myDFPlayer.play(")); Serial.print((uint16_t) mySound); Serial.println(F(")"));
  state_timerForceSoundActv = millis() + SOUND_ACTIVE_PROTECT; // handle YX5200 problem with interrupting play

  if (init_minmax) {
    init_minmax -= 1;
  }  else {
    diff_msec = now_msec - prev_msec;
    if (diff_msec > max_msec) {
      max_msec = diff_msec;
      // Serial.print(F("max_msec ")); Serial.println(max_msec);
    } else if (diff_msec < min_msec) {
      min_msec = diff_msec;
      // Serial.print(F("min_msec ")); Serial.println(min_msec);
    }
  }
  prev_msec = now_msec;
} // end DFstartSound()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFcheckSoundDone()
//
// notBusy means (HIGH == digitalRead(DPIN_AUDIO_BUSY)) && (millis() >= state_timerForceSoundActv)
//    DPIN_AUDIO_BUSY goes HIGH when sound finished, but may take a while to start
//    state_timerForceSoundActv is millisec count we have to wait for to before checking DPIN_AUDIO_BUSY
//
void DFcheckSoundDone() {
  uint8_t myBusy = (HIGH != digitalRead(DPIN_AUDIO_BUSY)) || (millis() < state_timerForceSoundActv);
  uint8_t playNumber = 99; // this means don't change

  if (0 != state_introSoundPlaying) { // intro still playing
    if (0 == myBusy) { // can play a non-intro sound
      if (0 != gPatternNumberChanged) { // start pattern sound
        playNumber = gCurrentPatternNumber+1; // sound numbers start at 1
      } else { // start intro sound
        playNumber = SOUNDNUM_INTRO; // this should never execute, we start with gPatternNumberChanged=1
      } // end start a sound
    } // end if can play a non-intro sound
  } else { // intro done
    if (0 != gPatternNumberChanged) { // always start new pattern number sound
      playNumber = gCurrentPatternNumber+1; // sound numbers start at 1
    }
  } // end if intro done

  if (99 != playNumber) { // there is a new sound to play
    gPatternNumberChanged = 0;
    state_introSoundPlaying = 0;
    if (SOUNDNUM_INTRO == playNumber) {
      DFstartSound(SOUNDNUM_INTRO, SOUND_BKGRND_VOL);
    } else {
      DFstartSound(gCurrentPatternNumber+1, SOUND_DEFAULT_VOL);
    }
  } // end if there is a new sound to play
} // end DFcheckSoundDone()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFsetup()
void DFsetup() {
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial, false, true)) {  // Use softwareSerial to communicate with mp3 player
    Serial.println(F("Unable to begin DFPlayer:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(1);
    }
  }
  myDFPlayer.EQ(DFPLAYER_EQ_BASS); // our speaker is quite small
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD); // device is SD card
  myDFPlayer.volume(SOUND_DEFAULT_VOL);  // Set volume value. From 0 to 30 - FIXME 25 is good
  delay(3000); // allow bluetooth connection to complete
  Serial.println(F("DFPlayer Mini online."));

  DFstartSound(SOUNDNUM_INTRO, SOUND_DEFAULT_VOL);
} // end DFsetup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// handle_ultra() - process HC-SR04 data.
//    returns: pattern number 0 <= num <= PATTERN_MAX_NUM
//

int handle_ultra() {
  int pattern; // integer pattern number from 0 thru 5 inclusive
  // get the range reading from the Ultrasonic sensor in centimeters
  int ultra_dist;
  
  gUltraDistance= (my_ultra.read(CM));
    ultra_dist = gUltraDistance - ULTRA_IGNORE_INITIAL_CM;
  if (ultra_dist < 0) ultra_dist = 0;
  pattern = ultra_dist / ULTRA_CM_PER_REGION;
  if (pattern > 5) pattern = 5;

  return(pattern);
} // end handle_ultra()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(""); // print a blank line in case there is some junk from power-on

  pinMode(DPIN_AUDIO_BUSY,  INPUT_PULLUP); // HIGH when audio stops
  mySoftwareSerial.begin(9600); // this is control to DFPlayer audio player
  // initialize the YX5200 DFPlayer audio player
  DFsetup();

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

#if DEBUG_CAPACITIVE_SETUP
  Serial.println("DEBUG_CAPACITIVE_SETUP");
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
#endif // DEBUG_CAPACITIVE_SETUP

  Serial.println("TODAS init complete...");
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
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
