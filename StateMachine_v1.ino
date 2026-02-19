#include <myDuino.h>
myDuino robot(1);

// ====== STATE MACHINE ======
enum State {
  PLUGGED,
  ACTIVE,
  COOLDOWN
};

State currentState = PLUGGED;

// ====== TIMING VARIABLES ======
unsigned long stateStartTime = 0;
const unsigned long ACTIVE_STATE_TIME = 40000;     // 40 seconds
const unsigned long COOLDOWN_STATE_TIME = 180000;  // 3 minutes

// ===== MOTOR VARIABLES =====
const int BIG_MOTOR_PIN = 1; // CHANGE INT IF NEEDED
const int SMALL_MOTOR_PIN = 2; // Remove if big motor is powerful enough
const int MOTOR_SPEED = 200; // CHANGE IF NEEDED
const int MOTOR_FORWARD = 1; // SWAP INT IF NEEDED
const int MOTOR_BACKWARD = 2;

// ====== BUTTON DEBOUNCE ======
const int BANANA_PLUG_PIN = 1; // CHANGE IF NEEDED
int bananaPlugState = 0;
int lastBananaPlugReading = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ====== ENCODER INTERRUPT ======
void encHandler() {
  robot.doEncoder();
}

void setup() {
  Serial.begin(9600);

  // ===== ENCODER SETUP =====
  attachInterrupt(digitalPinToInterrupt(2), encHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), encHandler, CHANGE);
}

void loop() {
  // ====== READ & DEBOUNCE BUTTON ======
  int bananaPlugReading = robot.readButton(BANANA_PLUG_PIN);

  if (bananaPlugReading != lastBananaPlugReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    bananaPlugState = bananaPlugReading;
  }
  lastBananaPlugReading = bananaPlugReading;

  // ====== STATE MACHINE ======
  switch (currentState) {

    case PLUGGED:
      Serial.println("in PLUGGED state");
      if (!bananaPlugState) { // Banana Plugs Disconnected
        currentState = ACTIVE; // Change state to ACTIVE
        stateStartTime = millis(); // Start 40s timer
        Serial.println("Started ACTIVE state");
      }

      break;

    case ACTIVE:
      Serial.println("in ACTIVE state");

      if (bananaPlugState) { // Banana Plugs Connected
        deActivateSystem(); // Stop all systems
        currentState = PLUGGED;
        Serial.println("BANANA PLUGS CONNECTED");
        break;
      }

      activateSystem(); // Move Motor while ACTIVE

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME) { // Begin cooldown after 40 secs
        deActivateSystem(); // Stop all systems
        currentState = COOLDOWN; // Change state to COOLDOWN (NOTE: MAY NOT BE NEEDED)
        stateStartTime = millis(); // Start 3 min timer
        Serial.println("Started COOLDOWN state");
      }

      break;

    case COOLDOWN: // MAY BE REDUNDANT FOR ACTIVATION TEST
      Serial.println("in COOLDOWN state");
      if (bananaPlugState) { // Banana Plugs Connected
        currentState = PLUGGED; // Go back to PLUGGED state
        Serial.println("BANANA PLUGS CONNECTED");
        break;
      }

      if (millis() - stateStartTime >= COOLDOWN_STATE_TIME) { // After 3 minutes, go back to "PLUGGED" state
        currentState = PLUGGED;
        Serial.println("COOLDOWN complete, returning to PLUGGED");
      }

      break;

    default:
      currentState = PLUGGED;
      break;
  }
  delay(50);
}

void activateSystem() { // Activate System Subsystem
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED);
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED); // DELETE IF NOT NEEDED
}

void deActivateSystem() { // Deactivate All Subsystems
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0); // DELETE IF NOT NEEDED
}

// ===== IMPLEMENT LATER =====
void deliverMario() {

}

void feedLumas() {

}

void stabilizeReactor() {

}

void defeatKoopas() {

}

