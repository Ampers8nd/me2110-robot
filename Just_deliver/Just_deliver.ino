#include <myDuino.h>

myDuino robot(1);

// ===== MOTOR VARIABLES =====
const int MOTOR_PIN = 2;
const int MOTOR_FORWARD = 1;
const int MOTOR_SPEED = 155;   // 0 to 255
const unsigned long RUN_TIME = 1600; // 2 seconds

bool motorStarted = false;
bool motorFinished = false;
unsigned long motorStartTime = 0;

void setup() {
  Serial.begin(9600);
  robot.moveMotor(MOTOR_PIN, MOTOR_FORWARD, 0); // make sure motor is off
  Serial.println("Motor test armed.");
}

void loop() {
  // Start motor once
  if (!motorStarted && !motorFinished) {
    motorStarted = true;
    motorStartTime = millis();
    robot.moveMotor(MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED);
    Serial.println("Motor started.");
  }

  // Stop motor after 2 seconds
  if (motorStarted && !motorFinished) {
    if (millis() - motorStartTime >= RUN_TIME) {
      robot.moveMotor(MOTOR_PIN, MOTOR_FORWARD, 0);
      motorFinished = true;
      Serial.println("Test complete. Motor stopped.");
    }
  }
}