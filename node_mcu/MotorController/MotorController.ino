//
// This file is part of the project open_the_door
// It is an automated door that can be opened either by SMS or by internet using a Google Cloud Run API
// Used to be uploaded to a NodeMCU Chip
//

#include <ESP8266WiFi.h>
#include "MotorController.h"

///////////////
//  GLOBALS  //
///////////////
String GInputString;
struct s_motor GMotorData = {.speed = MOTOR_DEFAULT_SPEED, .direction = MOTOR_DEFAULT_DIRECTION, .span= MOTOR_DEFAULT_SPAN, .motor = Stepper(MOTOR_STEPS, MOTOR_IN4, MOTOR_IN2, MOTOR_IN3, MOTOR_IN1)};
struct s_door GDoorData = {.isDoorOpened = false, .isLatchOpened = false, latchOpenedSince = 0};

////////////////
//  FUNCTIONS //
////////////////
const char *convert_bool_to_status(bool statusToConvert) {
  return (statusToConvert ? "Opened" : "Closed");
}

void run_status(struct s_motor* motorData, struct s_door* doorData) {
  Serial.print("Status of the door: ");
  Serial.println(convert_bool_to_status(doorData->isDoorOpened));
  Serial.print("Status of the latch: ");
  Serial.println(convert_bool_to_status(doorData->isLatchOpened));
  Serial.println("");
  Serial.println("Motor Status:");
  Serial.print("Direction: ");
  Serial.println(motorData->direction);
  Serial.print("Span: ");
  Serial.println(motorData->span);
  Serial.print("Speed: ");
  Serial.println(motorData->speed);
  Serial.println("");
}

void run_open_latch(struct s_motor* motorData, struct s_door* doorData) {
  if (doorData->isLatchOpened) {
    Serial.println("Latch already opened.");
    return;
  } else if (doorData->isDoorOpened) {
    Serial.println("Door already opened, it is not necessary to open the latch.");
    return;
  }
  Serial.println("Opening the latch...");
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span, motorData);
  doorData->isLatchOpened = true;
  doorData->latchOpenedSince = millis();
  digitalWrite(LED_PIN_RED, HIGH);
  Serial.println("Latch opened.");
}

void run_close_latch(struct s_motor* motorData, struct s_door* doorData) {
  if (!doorData->isLatchOpened) {
    Serial.println("Latch already closed.");
    return;
  }
  Serial.println("Closing the latch...");
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span * -1, motorData);
  doorData->isLatchOpened = false;
  doorData->latchOpenedSince = 0;
  digitalWrite(LED_PIN_RED, HIGH);
  Serial.println("Latch closed.");
}

void step_motor(int step, struct s_motor* motorData)
{
  ESP.wdtDisable();
  motorData->motor.setSpeed(motorData->speed);
  motorData->motor.step(motorData->direction * step);
  ESP.wdtEnable(DEFAULT_SOFTWARE_TIMEOUT);
}

void set_door_status(struct s_door* doorData) {
  doorData->isDoorOpened = !doorData->isDoorOpened;
  if (doorData->isDoorOpened) {
    Serial.println("Door opened.");
  } else {
    Serial.println("Door closed.");
  }
}

void set_motor_direction(String *direction, struct s_motor* motorData) {
  if (direction->equals("motor_direction=1")) {
    motorData->direction = 1;
  } else if (direction->equals("motor_direction=-1")) {
    motorData->direction = -1;
  } else {
    Serial.println("Invalid motor direction.");
    return;
  }
  Serial.print("Motor direction is set to: ");
  Serial.println(motorData->direction);
}

void set_motor_span(int newSpan, struct s_motor* motorData) {
  if (newSpan <= 0) {
    Serial.println("Invalid input motor span.");
    return;
  }
  motorData->span = newSpan;
  Serial.print("Motor span is set to: ");
  Serial.println(motorData->span);
}

void set_motor_speed(int newSpeed, struct s_motor* motorData) {
  if (newSpeed <= 0) {
    Serial.println("Invalid input motor speed.");
    return;
  }
  motorData->speed = newSpeed;
  Serial.print("Motor speed is set to: ");
  Serial.println(motorData->speed);
}

int getValueFromMessage(String *message, char charSeparator) {
  int index = 0;

  if (!message)
    return 0;
  index = message->indexOf(charSeparator);
  if (index == -1)
    return 0;
  String extract = message->substring(index + 1);
  return extract.toInt();
}

void interpretSerialMessage(String *message, struct s_motor* motorData, struct s_door* doorData) {
  if (message->equals("open")) {
    run_open_latch(motorData, doorData);
  } else if (message->equals("close")) {
    run_close_latch(motorData, doorData);
  } else if (message->equals("status")) {
    run_status(motorData, doorData);
  } else if (message->equals("door")) { // DEBUG This is a temporary command for debugging
    set_door_status(doorData);
  } else if (message->startsWith("motor_span=")) {
    set_motor_span(getValueFromMessage( message, '=' ), motorData);
  } else if (message->startsWith("motor_forward=")) {
    step_motor(getValueFromMessage( message, '=' ), motorData);
  } else if (message->startsWith("motor_backward=")) {
    step_motor(getValueFromMessage( message, '=' ) * -1, motorData);
  } else if (message->startsWith("motor_direction=")) {
    set_motor_direction(message, motorData);
  } else if (message->startsWith("motor_speed=")) {
    set_motor_speed(getValueFromMessage( message, '=' ), motorData);
  }
}

void close_latch_if_needed(struct s_motor* motorData, struct s_door* doorData) {
  unsigned long current_time = millis();

  if (current_time - doorData->latchOpenedSince >= TIME_MS_BEFORE_CLOSING_LATCH ) {
    Serial.println("--> Automatically closing latch.");
    run_close_latch(motorData, doorData);
  }
}

////////////
//  SETUP //
////////////
void setup() {
  // Basic init
  Serial.begin(115200);
  delay(10);
  Serial.setTimeout(300);
  Serial.println("");
  Serial.println("Initialization of the card...");
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_BLUE, OUTPUT);
  digitalWrite(LED_PIN_RED, LOW); // Enable the LED (indicates that it is starting up)
  digitalWrite(LED_PIN_BLUE, LOW);

  // Motor init
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);

  // Wifi init
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Connecting to Wifi
  Serial.print("Trying to establish connection to ");
  Serial.println(WIFI_SSID);
  /*while (WiFi.status() != WL_CONNECTED) { // TODO check wifi in the loop because it is not necessary to be able to open the door.
  //  Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP()); */

  // Door init
  // TODO get the status from the memory and/or the magnetic switch
  GDoorData.isDoorOpened = false;
  GDoorData.isLatchOpened = false;
  GDoorData.latchOpenedSince = 0;

  // Motor init
  // TODO get the variables from the memory

  run_status(&GMotorData, &GDoorData);
  Serial.println("Initialization finished.");
  digitalWrite(LED_PIN_RED, HIGH); // Disable the LED (indicates that it is ready to work)
}

////////////////
//  MAINLOOP  //
////////////////
void loop() {
  // TODO blink blue led to say it is ready to operate
  //digitalWrite(LED_PIN_RED, LOW);

  // TODO update boolean status of the door from captors

  close_latch_if_needed(&GMotorData, &GDoorData);

  GInputString = Serial.readString();
  if (GInputString.length() != 0) {
    interpretSerialMessage(&GInputString, &GMotorData, &GDoorData);
  }

  // TODO check socket message

  // TODO check SMS message (probably same as serial)

  //digitalWrite(LED_PIN_RED, HIGH);
}
