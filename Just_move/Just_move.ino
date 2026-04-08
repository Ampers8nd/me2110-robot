#include <myDuino.h>
myDuino robot(1);

bool triggered = false;
bool lastBtnState = false;
bool printed = false;
const unsigned long TIMEOUT = 3500; // 4 seconds in milliseconds

void setup() {
  Serial.begin(9600);
  lastBtnState = robot.readButton(1);
}

void loop() {
  bool btnState = robot.readButton(1);

  // Check for button trigger
  if (!triggered && btnState != lastBtnState) {
    triggered = true;
    Serial.println("Btn triggered, stopping.");
  }

  // Stop the motor if the button was triggered OR if 4 seconds have passed
  if (triggered || millis() >= TIMEOUT) {
    robot.moveMotor(1, 1, 0);
    if (triggered && !printed) {
      Serial.println("Button reached.");
      printed = true;
    } else if (millis() >= TIMEOUT && !printed) {
      Serial.println("TIMED OUT.");
      printed = true;
    }
  } else {
    robot.moveMotor(1, 1, 255);
  }

  lastBtnState = btnState;
}