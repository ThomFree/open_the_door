//
// The header file for the Open The Door project
//

#include <Stepper.h>

//////////////
//  CONFIG  //
//////////////
// HTTP SERVER
#define HTTP_SERVER_PORT 80

// MOTOR
#define MOTOR_DEFAULT_SPEED 10
#define MOTOR_DEFAULT_SPAN 1000
#define MOTOR_DEFAULT_DIRECTION 1
#define MOTOR_STEPS 2048
#define MOTOR_IN1 D5
#define MOTOR_IN2 D6
#define MOTOR_IN3 D7
#define MOTOR_IN4 D8

// LEDS
#define LED_PIN_RED 16
#define LED_PIN_BLUE 2
#define TIME_BETWEEN_BLINKS 800

// DOOR CAPTOR
#define CAPTOR_PIN D1
#define CAPTOR_OPEN_VALUE 0

// WIFI
#define WIFI_SSID "ouioui"
#define WIFI_PASSWORD "azertyuiop"
#define MAX_WIFI_CONNECT_DELAY 20000
#define WIFI_DELAY_BETWEEN_CONNECT 250

// DOOR
#define TIME_MS_BEFORE_CLOSING_LATCH 5000

// NODEMCU
#define DEFAULT_SOFTWARE_TIMEOUT 1000

// CONFIG
#define DEFAULT_MAGIC_NUMBER "42IsLife"
#define MAGIC_NUMBER_SIZE 9
#define EEPROM_STARTING_ADDRESS 0x00
#define MAX_WIFI_SSID_SIZE 33
#define MAX_WIFI_PASS_SIZE 64
#define NB_USERS 10
#define KEY_SIZE 20
#define USERNAME_SIZE 25

/////////////////
//  STRUCTURE  //
/////////////////
struct s_user {
  char key[KEY_SIZE];
  char username[USERNAME_SIZE];
};

struct s_motor {
  int speed;
  int direction;
  int span;
};

struct s_config {
  char magic_number[MAGIC_NUMBER_SIZE];
  char wifi_ssid[MAX_WIFI_SSID_SIZE];
  char wifi_password[MAX_WIFI_PASS_SIZE];
  struct s_user users[NB_USERS];
  struct s_motor motor;
};

struct s_door {
  bool isDoorOpened;
  bool isLatchOpened;
  unsigned long latchOpenedSince;
};
