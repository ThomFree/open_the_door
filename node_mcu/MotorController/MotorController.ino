//
// This file is part of the project open_the_door
// It is an automated door that can be opened either by SMS or by internet using a Web API
// Used to be uploaded to a NodeMCU Chip
//

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "MotorController.h"

///////////////
//  GLOBALS  //
///////////////
String GInputString;
struct s_config GConfig;
struct s_door GDoorData = {.isDoorOpened = false, .isLatchOpened = false, .latchOpenedSince = 0};
Stepper GMotor = Stepper(MOTOR_STEPS, MOTOR_IN4, MOTOR_IN2, MOTOR_IN3, MOTOR_IN1);


////////////////////////
//  MEMORY FUNCTIONS  //
////////////////////////
void print_motor_status(struct s_config *configData) {
  Serial.print("Motor direction: ");
  Serial.println(configData->motor.direction);
  Serial.print("Motor span: ");
  Serial.println(configData->motor.span);
  Serial.print("Motor speed: ");
  Serial.println(configData->motor.speed);
}

void print_user_status(struct s_user *user, int id) {
  Serial.print("User ");
  Serial.print(id + 1);
  Serial.print(" : ");
  if (user->key[0] == '\0') {
    Serial.println(" == Undefined ==");
    return;
  }
  Serial.print(user->username);
  Serial.print(" / ");
  Serial.println(user->key);
}

void print_all_users(struct s_config *configData) {
  for (int i = 0; i < NB_USERS; i++) {
    print_user_status(&configData->users[i], i);
  }
}

void print_global_config(struct s_config *configData) {
  Serial.println("[I] Current config:");
  Serial.print("Wifi: ");
  Serial.print(configData->wifi_ssid);
  Serial.print(" / ");
  Serial.println(configData->wifi_password);
  print_all_users(configData);
  print_motor_status(configData);
  Serial.println("[I] End of current config:");
}

void write_config_to_memory(struct s_config *configData) {
  char *tmp =  (char *)configData;

  for (int i = 0; i < sizeof(struct s_config); i++) {
    EEPROM.write(EEPROM_STARTING_ADDRESS + i, tmp[i]);
  }
  EEPROM.commit();
  delay(100);
}

void reset_global_config(struct s_config *configData) {
  memset(configData, '\0', sizeof(struct s_config));
  strcpy(configData->magic_number, DEFAULT_MAGIC_NUMBER);
  strcpy(configData->wifi_ssid, WIFI_SSID);
  strcpy(configData->wifi_password, WIFI_PASSWORD);
  configData->motor.speed = MOTOR_DEFAULT_SPEED;
  configData->motor.direction = MOTOR_DEFAULT_DIRECTION;
  configData->motor.span = MOTOR_DEFAULT_SPAN;
  Serial.println("[I] Config reset to default values successfully.");
}

void init_global_config(struct s_config *configData) {
  int data_size = sizeof(struct s_config);
  char *tmp =  (char *)configData;

  EEPROM.begin(data_size);
  for (int i = 0; i < data_size; i++) {
    tmp[i] = char(EEPROM.read(EEPROM_STARTING_ADDRESS + i));
  }
  if (strcmp(configData->magic_number, DEFAULT_MAGIC_NUMBER) != 0) {
    Serial.println("[I] The magic number is invalid. Resetting config to default.");
    reset_global_config(configData);
    write_config_to_memory(configData);
  }
  print_global_config(configData);
}

////////////////
//  FUNCTIONS //
////////////////
const char *convert_bool_to_status(bool statusToConvert) {
  return (statusToConvert ? "Opened" : "Closed");
}

void run_status(struct s_motor* motorData, struct s_door* doorData) {
  Serial.print("[I] Status of the door: ");
  Serial.println(convert_bool_to_status(doorData->isDoorOpened));
  Serial.print("[I] Status of the latch: ");
  Serial.println(convert_bool_to_status(doorData->isLatchOpened));
  Serial.println("");
}

void run_open_latch(struct s_motor* motorData, struct s_door* doorData) {
  if (doorData->isLatchOpened) {
    Serial.println("[W] Latch already opened.");
    return;
  } else if (doorData->isDoorOpened) {
    Serial.println("[W] Door already opened, it is not necessary to open the latch.");
    return;
  }
  Serial.println("[I] Opening the latch...");
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span, motorData);
  doorData->isLatchOpened = true;
  doorData->latchOpenedSince = millis();
  digitalWrite(LED_PIN_RED, HIGH);
  Serial.println("[I] Latch opened.");
}

void run_close_latch(struct s_motor* motorData, struct s_door* doorData) {
  if (!doorData->isLatchOpened) {
    Serial.println("[W] Latch already closed.");
    return;
  }
  Serial.println("[I] Closing the latch...");
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span * -1, motorData);
  doorData->isLatchOpened = false;
  doorData->latchOpenedSince = 0;
  digitalWrite(LED_PIN_RED, HIGH);
  Serial.println("[I] Latch closed.");
}

void step_motor(int step, struct s_motor* motorData)
{
  ESP.wdtDisable();
  GMotor.setSpeed(motorData->speed);
  GMotor.step(motorData->direction * step);
  ESP.wdtEnable(DEFAULT_SOFTWARE_TIMEOUT);
}

void set_door_status(struct s_door* doorData) {
  doorData->isDoorOpened = !doorData->isDoorOpened;
  if (doorData->isDoorOpened) {
    Serial.println("[I] Door opened.");
  } else {
    Serial.println("[I] Door closed.");
  }
}

void set_motor_direction(String *direction, struct s_motor* motorData) {
  if (direction->equals("motor_direction=1")) {
    motorData->direction = 1;
  } else if (direction->equals("motor_direction=-1")) {
    motorData->direction = -1;
  } else {
    Serial.println("[E] Invalid motor direction.");
    return;
  }
  Serial.print("[I] Motor direction is set to: ");
  Serial.println(motorData->direction);
}

void set_motor_span(int newSpan, struct s_motor* motorData) {
  if (newSpan <= 0) {
    Serial.println("[E] Invalid input motor span.");
    return;
  }
  motorData->span = newSpan;
  Serial.print("[I] Motor span is set to: ");
  Serial.println(motorData->span);
}

void set_motor_speed(int newSpeed, struct s_motor* motorData) {
  if (newSpeed <= 0) {
    Serial.println("[E] Invalid input motor speed.");
    return;
  }
  motorData->speed = newSpeed;
  Serial.print("[I] Motor speed is set to: ");
  Serial.println(motorData->speed);
}

void set_wifi_ssid(String *message, struct s_config *configData) {
  String ssid = getStringFromMessage(message, '=', 1);

  if (ssid.length() <= 0) {
    Serial.println("[E] Invalid Wifi SSID.");
    return;
  }
  if (ssid.length() >= MAX_WIFI_SSID_SIZE) {
    Serial.println("[E] Wifi SSID is too long.");
    return;
  }
  strcpy(configData->wifi_ssid, ssid.c_str());
  Serial.print("[I] Wifi SSID is set to: ");
  Serial.println(ssid);
}

void set_wifi_password(String *message, struct s_config *configData) {
  String pass = getStringFromMessage(message, '=', 1);

  if (pass.length() <= 0) {
    Serial.println("[E] Invalid Wifi Password.");
    return;
  }
  if (pass.length() >= MAX_WIFI_PASS_SIZE) {
    Serial.println("[E] Wifi password is too long.");
    return;
  }
  strcpy(configData->wifi_password, pass.c_str());
  Serial.print("[I] Wifi password is set to: ");
  Serial.println(pass);
}

void set_user(String *message, struct s_config *configData) {
  String id = getStringFromMessage(message, '=', 1);
  String username = getStringFromMessage(message, '=', 2);
  String key = getStringFromMessage(message, '=', 3);
  String last = getStringFromMessage(message, '=', 4);
  int pos = id.toInt() - 1;

  if (id.length() <= 0 || username.length() <= 0 || key.length() <= 0 || last.length() != 0) {
    Serial.println("[E] Invalid Set user request. Format: id=username=key (You can't have '=' in username or key)");
    return;
  }
  if (username.length() >= USERNAME_SIZE || key.length() >= KEY_SIZE) {
    Serial.println("[E] The key or username you provided is too long.");
    return;
  }
  if (pos < 0 || pos > 9) {
    Serial.print("[E] Invalid ID, it has to be between >= 1 and <= ");
    Serial.print(NB_USERS);
    Serial.println(".");
    return;
  }
  strcpy(configData->users[pos].key, key.c_str());
  strcpy(configData->users[pos].username, username.c_str());
  Serial.println("[I] User set.");
}

void remove_user(struct s_config *configData, int id) {
  if (id < 1 || id > NB_USERS) {
    Serial.print("[E] Invalid ID, it has to be between >= 1 and <= ");
    Serial.print(NB_USERS);
    Serial.println(".");
    return;
  }
  id -= 1;
  memset(configData->users[id].key, '\0', KEY_SIZE);
  memset(configData->users[id].username, '\0', USERNAME_SIZE);
  Serial.print("[I] User ");
  Serial.print(id + 1);
  Serial.println(" successfully removed.");
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

String getStringFromMessage(String *message, char charSeparator, int skip) {
  int index = 0;
  String result = "";
  String tmp = message->substring(0);

  if (!message || skip < 1)
    return result;
  for (int i = 0; i < skip; i++) {
    index = tmp.indexOf(charSeparator);
    if (index == -1)
      return result;
    tmp = tmp.substring(index + 1);
  }
  index = tmp.indexOf(charSeparator);
  if (index == -1)
    return tmp;
  tmp = tmp.substring(0, index);
  return tmp;
}

void interpretSerialMessage(String *message, struct s_config* configData, struct s_door* doorData) {
  struct s_motor* motorData = &configData->motor;

  if (message->equals("open")) {
    run_open_latch(motorData, doorData);
  } else if (message->equals("close")) {
    run_close_latch(motorData, doorData);
  } else if (message->startsWith("reset_config")) {
    reset_global_config(configData);
    write_config_to_memory(configData);
  } else if (message->equals("motor_status")) {
    print_motor_status(configData);
  } else if (message->equals("config_status")) {
    print_global_config(configData);
  } else if (message->equals("status")) {
    run_status(motorData, doorData);
  } else if (message->startsWith("motor_span=")) {
    set_motor_span(getValueFromMessage( message, '=' ), motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("motor_forward=")) {
    step_motor(getValueFromMessage( message, '=' ), motorData);
  } else if (message->startsWith("motor_backward=")) {
    step_motor(getValueFromMessage( message, '=' ) * -1, motorData);
  } else if (message->startsWith("motor_direction=")) {
    set_motor_direction(message, motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("motor_speed=")) {
    set_motor_speed(getValueFromMessage( message, '=' ), motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("wifi_ssid=")) {
    set_wifi_ssid(message, configData);
    write_config_to_memory(configData);
  } else if (message->startsWith("wifi_password=")) {
    set_wifi_password(message, configData);
    write_config_to_memory(configData);
  } else if (message->startsWith("set_user=")) {
    set_user(message, configData);
    write_config_to_memory(configData);
  } else if (message->startsWith("remove_user=")) {
    remove_user(configData, getValueFromMessage( message, '=' ));
    write_config_to_memory(configData);
  } else {
    Serial.println("[W] Command not found.");
  }
}

void close_latch_if_needed(struct s_motor* motorData, struct s_door* doorData) {
  unsigned long current_time = millis();

  if (doorData->latchOpenedSince != 0 && current_time - doorData->latchOpenedSince >= TIME_MS_BEFORE_CLOSING_LATCH ) {
    Serial.println("[I] Automatically closing latch...");
    run_close_latch(motorData, doorData);
    Serial.println("[I] Automatically closing latch done.");
  }
}

void check_door_captor(struct s_door* doorData) {
  int value = digitalRead(CAPTOR_PIN);

  if (value == CAPTOR_OPEN_VALUE) {
    doorData->isDoorOpened = true;
  } else {
    doorData->isDoorOpened = false;
  }
}

////////////
//  SETUP //
////////////
void setup() {
  // Mandatory delay or the board will not boot
  delay(500);
  // Basic init
  Serial.begin(115200);
  delay(10);
  Serial.setTimeout(300);
  Serial.println("");
  // Memory init
  init_global_config(&GConfig);
  Serial.println("[I] Initialization of the card...");
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_BLUE, OUTPUT);
  digitalWrite(LED_PIN_RED, LOW); // Enable the LED (indicates that it is starting up)
  digitalWrite(LED_PIN_BLUE, LOW);

  // Captor init
  pinMode(CAPTOR_PIN, INPUT);

  // Motor init
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);

  // Wifi init
  WiFi.begin(GConfig.wifi_ssid, GConfig.wifi_password); // Connecting to Wifi
  Serial.print("[I] Trying to establish connection to ");
  Serial.println(GConfig.wifi_ssid);
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

  run_status(&GConfig.motor, &GDoorData);
  Serial.println("[I] Initialization finished.");
  digitalWrite(LED_PIN_RED, HIGH); // Disable the LED (indicates that it is ready to work)
}

////////////////
//  MAINLOOP  //
////////////////
void loop() {
  // TODO blink blue led to say it is ready to operate
  //digitalWrite(LED_PIN_RED, LOW);
  //digitalWrite(LED_PIN_RED, HIGH);

  check_door_captor(&GDoorData);
  close_latch_if_needed(&GConfig.motor, &GDoorData);

  GInputString = Serial.readString();
  if (GInputString.length() != 0) {
    interpretSerialMessage(&GInputString, &GConfig, &GDoorData);
  }

  // TODO check socket message

  // TODO check SMS message (probably same as serial)
}
