//
// This file is part of the project open_the_door
// It is an automated door that can be opened either by SMS or by internet using a Web API
// Used to be uploaded to a NodeMCU Chip
//

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include "MotorController.h"

///////////////
//  GLOBALS  //
///////////////
String GInputString;
struct s_config GConfig;
struct s_door GDoorData = {.isDoorOpened = false, .isLatchOpened = false, .latchOpenedSince = 0};
Stepper GMotor = Stepper(MOTOR_STEPS, MOTOR_IN4, MOTOR_IN2, MOTOR_IN3, MOTOR_IN1);
AsyncWebServer GWebServer(HTTP_SERVER_PORT);

// Keep alive led for main loop
bool GAliveLed = false;
unsigned long GLastBlink = 0;


////////////////////////
//  MEMORY FUNCTIONS  //
////////////////////////
String print_motor_status(struct s_config *configData) {
  String res;

  res += "Motor direction: ";
  res += configData->motor.direction;
  res += "\nMotor span: ";
  res += configData->motor.span;
  res += "\nMotor speed: ";
  res += configData->motor.speed;
  res += "\n";
  return res;
}

String print_user_status(struct s_user *user, int id) {
  String res;

  res += "User ";
  res += id + 1;
  res += " : ";
  if (user->key[0] == '\0') {
    res += " == Undefined ==";
    return res;
  }
  res += user->username;
  res += " / ";
  res += user->key;
  return res;
}

String print_all_users(struct s_config *configData) {
  String res;

  for (int i = 0; i < NB_USERS; i++) {
    res += print_user_status(&configData->users[i], i);
  }
  return res;
}

String print_global_config(struct s_config *configData) {
  String res;

  res += "[I] Current config:\n";
  res += "Wifi: ";
  res += configData->wifi_ssid;
  res += " / ";
  res += configData->wifi_password;
  res += print_all_users(configData);
  res += print_motor_status(configData);
  res += "\n[I] End of current config\n";
  return res;
}

void write_config_to_memory(struct s_config *configData) {
  char *tmp =  (char *)configData;

  for (int i = 0; i < sizeof(struct s_config); i++) {
    EEPROM.write(EEPROM_STARTING_ADDRESS + i, tmp[i]);
  }
  EEPROM.commit();
  delay(100);
}

String reset_global_config(struct s_config *configData) {
  memset(configData, '\0', sizeof(struct s_config));
  strcpy(configData->magic_number, DEFAULT_MAGIC_NUMBER);
  strcpy(configData->wifi_ssid, WIFI_SSID);
  strcpy(configData->wifi_password, WIFI_PASSWORD);
  configData->motor.speed = MOTOR_DEFAULT_SPEED;
  configData->motor.direction = MOTOR_DEFAULT_DIRECTION;
  configData->motor.span = MOTOR_DEFAULT_SPAN;
  return(String("[I] Config reset to default values successfully.\n"));
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

String run_status(struct s_motor* motorData, struct s_door* doorData) {
  String res;

  res += "[I] Status of the door: ";
  res += convert_bool_to_status(doorData->isDoorOpened);
  res += "\n[I] Status of the latch: ";
  res += convert_bool_to_status(doorData->isLatchOpened);
  res += "\n\n";
  return res;
}

String wifi_status() {
  String res;

  if (WiFi.status() == WL_CONNECTED) {
    res += "[I] WiFi connected. IP: ";
    res += WiFi.localIP().toString();
    res += "\n";
  } else {
    res += "[I] Wifi not connected.\n";
  }
  return res;
}

String run_open_latch(struct s_motor* motorData, struct s_door* doorData) {
  String res;

  if (doorData->isLatchOpened) {
    res += "[W] Latch already opened.\n";
    return res;
  } else if (doorData->isDoorOpened) {
    res += "[W] Door already opened, it is not necessary to open the latch.\n";
    return res;
  }
  res += "[I] Opening the latch...\n";
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span, motorData);
  doorData->isLatchOpened = true;
  doorData->latchOpenedSince = millis();
  digitalWrite(LED_PIN_RED, HIGH);
  res += "[I] Latch opened.\n";
  return res;
}

String run_close_latch(struct s_motor* motorData, struct s_door* doorData) {
  String res;

  if (!doorData->isLatchOpened) {
    res += "[W] Latch already closed.\n";
    return res;
  }
  res += "[I] Closing the latch...\n";
  digitalWrite(LED_PIN_RED, LOW);
  step_motor(motorData->span * -1, motorData);
  doorData->isLatchOpened = false;
  doorData->latchOpenedSince = 0;
  digitalWrite(LED_PIN_RED, HIGH);
  res += "[I] Latch closed.\n";
  return res;
}

void step_motor(int step, struct s_motor* motorData)
{
  ESP.wdtDisable();
  GMotor.setSpeed(motorData->speed);
  GMotor.step(motorData->direction * step);
  ESP.wdtEnable(DEFAULT_SOFTWARE_TIMEOUT);
}

String set_door_status(struct s_door* doorData) {
  String res;

  doorData->isDoorOpened = !doorData->isDoorOpened;
  if (doorData->isDoorOpened) {
    res += "[I] Door opened.\n";
  } else {
    res += "[I] Door closed.\n";
  }
  return res;
}

String set_motor_direction(String *direction, struct s_motor* motorData) {
  String res;

  if (direction->equals("motor_direction=1")) {
    motorData->direction = 1;
  } else if (direction->equals("motor_direction=-1")) {
    motorData->direction = -1;
  } else {
    res += "[E] Invalid motor direction.\n";
    return res;
  }
  res += "[I] Motor direction is set to: ";
  res += motorData->direction;
  res += "\n";
  return res;
}

String set_motor_span(int newSpan, struct s_motor* motorData) {
  String res;

  if (newSpan <= 0) {
    return String("[E] Invalid input motor span.\n");
  }
  motorData->span = newSpan;
  res += "[I] Motor span is set to: ";
  res += motorData->span;
  res += "\n";
  return res;
}

String set_motor_speed(int newSpeed, struct s_motor* motorData) {
  String res;

  if (newSpeed <= 0) {
    res += "[E] Invalid input motor speed.";
    return res;
  }
  motorData->speed = newSpeed;
  res += "[I] Motor speed is set to: ";
  res += motorData->speed;
  res += "\n";
  return res;
}

String set_wifi_ssid(String *message, struct s_config *configData) {
  String ssid = getStringFromMessage(message, '=', 1);
  String res;

  if (ssid.length() <= 0) {
    return String("[E] Invalid Wifi SSID.\n");
  }
  if (ssid.length() >= MAX_WIFI_SSID_SIZE) {
    return String("[E] Wifi SSID is too long.\n");
  }
  strcpy(configData->wifi_ssid, ssid.c_str());
  res += "[I] Wifi SSID is set to: ";
  res += ssid;
  res += "\n";
  return res;
}

String set_wifi_password(String *message, struct s_config *configData) {
  String pass = getStringFromMessage(message, '=', 1);
  String res;

  if (pass.length() <= 0) {
    return String("[E] Invalid Wifi Password.");
  }
  if (pass.length() >= MAX_WIFI_PASS_SIZE) {
    return String("[E] Wifi password is too long.");
  }
  strcpy(configData->wifi_password, pass.c_str());
  res += "[I] Wifi password is set to: ";
  res += pass;
  res += "\n";
  return res;
}

String set_user(String *message, struct s_config *configData) {
  String id = getStringFromMessage(message, '=', 1);
  String username = getStringFromMessage(message, '=', 2);
  String key = getStringFromMessage(message, '=', 3);
  String last = getStringFromMessage(message, '=', 4);
  String res;
  int pos = id.toInt() - 1;

  if (id.length() <= 0 || username.length() <= 0 || key.length() <= 0 || last.length() != 0) {
    return String("[E] Invalid Set user request. Format: id=username=key (You can't have '=' in username or key)\n");
  }
  if (username.length() >= USERNAME_SIZE || key.length() >= KEY_SIZE) {
    return String("[E] The key or username you provided is too long.\n");
  }
  if (pos < 0 || pos > 9) {
    res += "[E] Invalid ID, it has to be between >= 1 and <= ";
    res += (NB_USERS);
    res += ".\n";
    return res;
  }
  strcpy(configData->users[pos].key, key.c_str());
  strcpy(configData->users[pos].username, username.c_str());
  res += "[I] User set.\n";
  return res;
}

String remove_user(struct s_config *configData, int id) {
  String res;

  if (id < 1 || id > NB_USERS) {
    res += "[E] Invalid ID, it has to be between >= 1 and <= ";
    res += (NB_USERS);
    res += ".\n";
    return res;
  }
  id -= 1;
  memset(configData->users[id].key, '\0', KEY_SIZE);
  memset(configData->users[id].username, '\0', USERNAME_SIZE);
  res += "[I] User ";
  res += id + 1;
  res += " successfully removed.\n";
  return res;
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

String interpretSerialMessage(String *message, struct s_config* configData, struct s_door* doorData) {
  struct s_motor* motorData = &configData->motor;
  String res = "yolo\n\nTEST\n\n";

  if (message->equals("open")) {
    res = run_open_latch(motorData, doorData);
  } else if (message->equals("close")) {
    res = run_close_latch(motorData, doorData);
  } else if (message->startsWith("reset_config")) {
    res = reset_global_config(configData);
    write_config_to_memory(configData);
  } else if (message->equals("motor_status")) {
    res = print_motor_status(configData);
  } else if (message->equals("config_status")) {
    res = print_global_config(configData);
  } else if (message->equals("status")) {
    res = run_status(motorData, doorData);
  } else if (message->startsWith("motor_span=")) {
    res = set_motor_span(getValueFromMessage( message, '=' ), motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("motor_forward=")) {
    step_motor(getValueFromMessage( message, '=' ), motorData);
    res = String("[I] Motor stepped forward.");
  } else if (message->startsWith("motor_backward=")) {
    step_motor(getValueFromMessage( message, '=' ) * -1, motorData);
    res = String("[I] Motor stepped backward.");
  } else if (message->startsWith("motor_direction=")) {
    res = set_motor_direction(message, motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("motor_speed=")) {
    res = set_motor_speed(getValueFromMessage( message, '=' ), motorData);
    write_config_to_memory(configData);
  } else if (message->startsWith("wifi_ssid=")) {
    res = set_wifi_ssid(message, configData);
    write_config_to_memory(configData);
    WiFi.begin(configData->wifi_ssid, configData->wifi_password);
  } else if (message->startsWith("wifi_password=")) {
    res = set_wifi_password(message, configData);
    write_config_to_memory(configData);
    WiFi.begin(configData->wifi_ssid, configData->wifi_password);
  } else if (message->startsWith("set_user=")) {
    res = set_user(message, configData);
    write_config_to_memory(configData);
  } else if (message->startsWith("remove_user=")) {
    res = remove_user(configData, getValueFromMessage( message, '=' ));
    write_config_to_memory(configData);
  } else if (message->startsWith("wifi_status")) {
    res = wifi_status();
  } else {
    res = String("[W] Command not found.");
  }
  return res;
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
  delay(1000);
  // Basic init
  Serial.begin(115200);
  delay(10);
  Serial.setTimeout(300);
  Serial.println("");
  Serial.println("[I] Initialization of the card...");
  // Memory init
  init_global_config(&GConfig);
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
  int max_delay = 0;
  while (WiFi.status() != WL_CONNECTED && max_delay < MAX_WIFI_CONNECT_DELAY) {
    Serial.print(".");
    delay(WIFI_DELAY_BETWEEN_CONNECT);
    max_delay += WIFI_DELAY_BETWEEN_CONNECT;
  }
  Serial.println(".");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[E] Wifi not connected, we will try again later. Please, in the meantime, verify the wifi credentials.");
  } else {
    Serial.print("[I] WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  }

  // Door init
  GDoorData.isDoorOpened = false;
  GDoorData.isLatchOpened = false;
  GDoorData.latchOpenedSince = 0;

  // HTTP Server init
  GWebServer.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if (request->url() == "/command" && request->method() == HTTP_POST) {
      String str = "";
      for(size_t i=0; i<len; i++) {
        str += (char)data[i];
      }
      Serial.println("--> Web server request command :");
      Serial.println(str.c_str());
      str = interpretSerialMessage(&str, &GConfig, &GDoorData);
      Serial.println("--> Web server request result:");
      Serial.println(str.c_str());
      Serial.println("--> End of web server request");
      request->send(200, "text/plain", str.c_str());
    }
  });
  GWebServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "The HTTP server is up and running!");
  });
  GWebServer.begin();

  run_status(&GConfig.motor, &GDoorData);
  Serial.println("[I] Initialization finished.");
  digitalWrite(LED_PIN_RED, HIGH); // Disable the LED (indicates that it is ready to work)
}

////////////////
//  MAINLOOP  //
////////////////
void loop() {
  String output;
  unsigned long timer = millis();

  // Blinker (led) act as a keep alive display
  if (timer - GLastBlink > TIME_BETWEEN_BLINKS) {
    digitalWrite(LED_PIN_BLUE, GAliveLed);
    GAliveLed = !GAliveLed;
    GLastBlink = timer;
  }

  check_door_captor(&GDoorData);
  close_latch_if_needed(&GConfig.motor, &GDoorData);

  GInputString = Serial.readString();
  if (GInputString.length() != 0) {
    output = interpretSerialMessage(&GInputString, &GConfig, &GDoorData);
    Serial.print(output);
  }

  // TODO check socket message

  // TODO check SMS message (probably same as serial)
}
