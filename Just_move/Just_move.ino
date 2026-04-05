#include <myDuino.h>
myDuino robot(1);

bool triggered = false;
const float tgt_dist = 7.5;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  float us_value = robot.readUltrasonic();
  if (us_value < 5.5) {
    robot.moveMotor(1, 1, 0);
  } else {
    robot.moveMotor(1,1,200);
  }
}
