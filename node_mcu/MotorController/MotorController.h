//
// The header file for the Open The Door project
//

#include <Stepper.h>

//////////////
//  CONFIG  //
//////////////
// MOTOR
#define MOTOR_DEFAULT_SPEED 10
#define MOTOR_DEFAULT_SPAN 500
#define MOTOR_DEFAULT_DIRECTION 1
#define MOTOR_STEPS 2048
#define MOTOR_IN1 D5
#define MOTOR_IN2 D6
#define MOTOR_IN3 D7
#define MOTOR_IN4 D8

// LEDS
#define LED_PIN_RED 16
#define LED_PIN_BLUE 2

// WIFI
//#define WIFI_SSID "Le Bureau"
//#define WIFI_PASSWORD "lespotdetravail2020"
#define WIFI_SSID "ouioui"
#define WIFI_PASSWORD "azertyuiop"

// DOOR
#define TIME_MS_BEFORE_CLOSING_LATCH 3000

// NODEMCU
#define DEFAULT_SOFTWARE_TIMEOUT 1000

/////////////////
//  STRUCTURE  //
/////////////////
struct s_motor {
  int speed;
  int direction;
  int span;
  Stepper motor;
};

struct s_door {
  bool isDoorOpened;
  bool isLatchOpened;
  unsigned long latchOpenedSince;
};
