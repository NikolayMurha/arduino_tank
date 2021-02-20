#include <GyverMotor.h>
#include <PS2X_lib.h>
#include "GyverTimer.h"

// Контроллер
#define PS2_DAT        12
#define PS2_CMD        13
#define PS2_ATT        6
#define PS2_CLK        7
int error = 0;
byte type = 0;

// Левый мотор
#define MOTOR_L 11
#define MOTOR_L_PWM 10
// Правый мотор
#define MOTOR_R 8
#define MOTOR_R_PWM 9

// Башня
#define MOTOR_HEAD_LEFT A2 //IN3
#define MOTOR_HEAD_RIGHT A3 // IN4
// Пушка
#define MOTOR_GUN_UP A1 //IN1 UP
#define MOTOR_GUN_DOWN A0 //IN2 UP
#define MOTOR_SHOT A4 // Выстрел
#define LIGHT A5 // Свет

int debounceTimeout = 200;
long lightTime = 0;
int lightState = 0;
int lighButtontPreviousState = 0;
long attentionTime = 0;
bool attentionEnabled = false;
int attentionButtonPreviousState = 0;
PS2X ps2x;
GMotor motorL(DRIVER2WIRE, MOTOR_L, MOTOR_L_PWM, HIGH);
GMotor motorR(DRIVER2WIRE, MOTOR_R, MOTOR_R_PWM, HIGH);
GMotor motorHead(RELAY2WIRE, MOTOR_HEAD_RIGHT, MOTOR_HEAD_LEFT, HIGH);
GMotor motorGun(RELAY2WIRE, MOTOR_GUN_UP, MOTOR_GUN_DOWN, HIGH);
GTimer attentionTimer(MS);

void initPins();
void initGamePad();
void initMotors();
void handleMotors();
void handleHead();
void handleLight();

void setup() {  
  Serial.begin(9600);
  initPins();
  initGamePad();
  initMotors();
  attentionTimer.setInterval(700);
}

void loop() {
  ps2x.read_gamepad(false, 0);
  handleMotors();
  handleHead();
  handleLight();
  delay(100);  // задержка просто для "стабильности"
}

void initPins() {
  pinMode(MOTOR_SHOT, OUTPUT);
  pinMode(LIGHT, OUTPUT);  
}

void initGamePad() {
   error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT, false, false);
  if (error == 0) {
    Serial.println("Found Controller, configured successful");
  }
  else if (error == 1) 
  {
    Serial.println("No controller found, check wiring or reset the Arduino");
  }
  else if (error == 2)
  {
    Serial.println("Controller found but not accepting commands");
  }
  else if (error == 3) 
  {
    Serial.println("Controller refusing to enter Pressures mode, may not support it.");
  }
  // Check for the type of controller
  type = ps2x.readType();
  switch (type) {
    case 0:
      Serial.println("Unknown Controller type");
      break;
    case 1:
      Serial.println("DualShock Controller Found");
      break;
    case 2:
      Serial.println("GuitarHero Controller Found");
      break;
  }
}

void initMotors() {
 // TCCR1A = 0b00000011;  // 10bit
 // TCCR1B = 0b00001001;  // x1 fast pwm
  motorL.setResolution(10);
  motorR.setResolution(10);
  motorR.setMode(FORWARD);
  motorL.setMode(FORWARD);
  motorL.setSpeed(0);
  motorR.setSpeed(0);
  // motorL.setMinDuty(500);
  // motorR.setMinDuty(500);
  motorHead.setMode(FORWARD);
  motorHead.setSpeed(0);
  motorGun.setMode(FORWARD);
  motorGun.setSpeed(0);
}

void handleMotors() { 
  int vX, vY;
  int padUp = ps2x.Button(PSB_PAD_UP);
  int padDown = ps2x.Button(PSB_PAD_DOWN);
  int padLeft = ps2x.Button(PSB_PAD_LEFT);
  int padRight = ps2x.Button(PSB_PAD_RIGHT);
  
  if (padLeft || padRight || padUp || padDown){
    vX = 127;
    vY = 127;
    if (padUp) { vY = 0; }
    if (padDown) { vY = 255; }
    if (padRight) { vX = 255; }
    if (padLeft) { vX = 0; }
  } else {
    vX = ps2x.Analog(PSS_LX);
    vY = ps2x.Analog(PSS_LY); 
  }
  vX = map(vX, 0, 255, 1023, -1023);
  vY = map(vY, 0, 255, 1023, -1023);  
  if (vY > -10 && vY < 10) { vY = 0; }
  if (vX > -10 && vX < 10) { vX = 0; }
  motorL.setSpeed(vY - vX);
  motorR.setSpeed(vY + vX);
}



void handleHead() {
  int vX = ps2x.Analog(PSS_RX);
  int vY = ps2x.Analog(PSS_RY);
 
  if(vX > 200){
    vX  = 1;
  } else if(vX < 100){
    vX = -1;
  }  else {
    vX = 0;
  }

  if(vY > 200){
    vY  = 1;
  } else if(vY < 100){
    vY = -1;
  }  else {
    vY = 0;
  }
  Serial.println(vX);
  Serial.println(vY);
  
  motorHead.setSpeed(vX);
  motorGun.setSpeed(vY);
  if ( ps2x.Button(PSB_R2) && ps2x.Button(PSB_L2)) {
    digitalWrite(MOTOR_SHOT, HIGH);
  } else {
    digitalWrite(MOTOR_SHOT, LOW);
  }
}


void handleLight() {
  int lightButton = ps2x.Button(PSB_GREEN);
  if (lightButton == HIGH && lighButtontPreviousState == LOW && millis() - lightTime > debounceTimeout) {
    lightState = lightState == HIGH ? LOW : HIGH;
    attentionEnabled = false;
    digitalWrite(LIGHT, lightState);
    Serial.print("Set light: ");
    Serial.println(lightState);
    lightTime = millis();
  }
  lighButtontPreviousState = lightButton;

  int attentionButton = ps2x.Button(PSB_PINK);
  if (attentionButton == HIGH && attentionButtonPreviousState == LOW && millis() - attentionTime > debounceTimeout) {    
    attentionEnabled = !attentionEnabled;
    Serial.print("Attention: ");
    Serial.println(attentionEnabled);
    attentionTime = millis();
    if (!attentionEnabled){
      digitalWrite(LIGHT, lightState);
     }
  }
  attentionButtonPreviousState = attentionButton;

  if (attentionEnabled && attentionTimer.isReady()) {
    lightState = lightState == HIGH ? LOW : HIGH;
    Serial.println("Blink");
    digitalWrite(LIGHT, lightState);
  }
}
