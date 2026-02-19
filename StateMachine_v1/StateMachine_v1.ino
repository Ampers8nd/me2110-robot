  #include <myDuino.h>
myDuino robot(1);

// ====== STATE MACHINE ======
enum State {
  WAITING,   // Waiting for banana plugs to be connected
  ACTIVE,    // Running for 40 seconds
  COOLDOWN   // Forced 3-minute shutdown
};

State currentState = WAITING;

// ====== TIMING VARIABLES ======
unsigned long stateStartTime = 0;
unsigned long motorStartTime = 0;
bool motorStartSet = false;
bool motorEndAnnounced = false;
const unsigned long MOTOR_ACTIVE_TIME = 5000; // 5 seconds
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
  // Note: This logic assumes bananaPlugState == 1 means CONNECTED. 
  
  switch (currentState) {

    case WAITING:
      deActivateSystem(); // Ensure system does absolutely nothing

      if (bananaPlugState == 1) { // Banana Plugs connected for the first time
        currentState = ACTIVE;
        stateStartTime = millis(); // Start 40s timer
        Serial.println("Banana Plugs Connected. Started ACTIVE state.");
        if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
        }
        activateSystem(motorStartTime);
        // Serial.println(motorStartTime);
      }

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME) { // 40 seconds reached, potential loss of banana plug power along the way
        Serial.println("40 seconds reached. Entering COOLDOWN.");
        deActivateSystem(); // Stop all systems regardless of plug state
        currentState = COOLDOWN;
        stateStartTime = millis(); // Start 3 min timer
      }
      break;

    case ACTIVE:
      if (bananaPlugState == 0) { // Banana Plugs disconnected prematurely
        Serial.println("Banana Plugs Disconnected. Deactivating system.");
        deActivateSystem();
        currentState = WAITING; // Return to waiting state
        break;
      }

      if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
      }

      activateSystem(motorStartTime); // Keep motors running while active
      // is there a chance motorStartTime here can be anything that isn't what we want it to be?

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME) { // 40 seconds reached
        Serial.println("40 seconds reached. Entering COOLDOWN.");
        deActivateSystem(); // Stop all systems regardless of plug state
        currentState = COOLDOWN;
        stateStartTime = millis(); // Start 3 min timer
      }
      break;

    case COOLDOWN:
      deActivateSystem(); // Enforce shutdown
      
      // We don't check bananaplugs here because they don't matter
      // hopefully this code doesnt crap itself when we change the cooldown.

      if (millis() - stateStartTime >= COOLDOWN_STATE_TIME) { // After 3 minutes
        currentState = WAITING;
        Serial.println("COOLDOWN complete, returning to WAITING.");
      }

      // reset motor active set flag
      motorStartSet = false;
      break;

    default:
      currentState = WAITING;
      break;
  }
  
  delay(50);
}

void activateSystem(unsigned long motorStartTime) { // Activate System Subsystem
  if (millis() - motorStartTime < MOTOR_ACTIVE_TIME) {
    robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED);
    // robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED); // DELETE IF NOT NEEDED
  } else {
    if (!motorEndAnnounced) {
      Serial.println("Motor Time Limit reached.");
      robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);
      motorEndAnnounced = true;
    }
  }
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED); // DELETE IF NOT NEEDED
}

void deActivateSystem() { // Deactivate All Subsystems
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0); // DELETE IF NOT NEEDED
}

// ===== IMPLEMENT LATER =====
void deliverMario() {}
void feedLumas() {}
void stabilizeReactor() {}
void defeatKoopas() {}