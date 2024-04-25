#include "Ultrasonic.h"

#define ULTRA_TRIG_PIN 12 // HC-SR04 Trigger digital pin
#define ULTRA_ECHO_PIN 10 // HC-SR04 Trigger echo pin

// instantiate my HC-SR04 data object
Ultrasonic my_ultra = Ultrasonic(ULTRA_TRIG_PIN, ULTRA_ECHO_PIN); // default timeout is 20 milliseconds

#define ULTRA_CM_PER_REGION 9 // HC-SR04 every this many CM is a different pattern
#define ULTRA_IGNORE_INITIAL_CM 3 // HC-SR04 ignore the first 3 CM since valid range starts at 2 CM
#define PATTERN_MAX_NUM 5 // maximum pattern number we have

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// handle_ultra() - process HC-SR04 data.
//    returns: pattern number 0 <= num <= PATTERN_MAX_NUM
//

int handle_ultra() {
  int pattern; // integer pattern number from 0 thru 5 inclusive
  // get the range reading from the Ultrasonic sensor in centimeters
  int ultra_dist=(my_ultra.read(CM));
  
  ultra_dist -= ULTRA_IGNORE_INITIAL_CM;
  if (ultra_dist < 0) ultra_dist = 0;
  pattern = ultra_dist / ULTRA_CM_PER_REGION;
  if (pattern > PATTERN_MAX_NUM) pattern = PATTERN_MAX_NUM;

  return(pattern);
} // end handle_ultra()

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
