#include <myDuino.h>
myDuino robot(1);

const unsigned long koopaActiveTimer = 5000;
const int koopaPin = 2;

bool koopaDeployed = false;
bool koopaRetracted = false;
unsigned long startTime = 0;

void setup() {
}

void loop() {
  if (!koopaDeployed) {
    robot.digital(koopaPin, 1);
    startTime = millis();
    koopaDeployed = true;
  }

  if (koopaDeployed && !koopaRetracted && millis() - startTime >= koopaActiveTimer) {
    robot.digital(koopaPin, 0);
    koopaRetracted = true;
  }
}