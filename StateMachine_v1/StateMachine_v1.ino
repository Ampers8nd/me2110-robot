#include <myDuino.h>
myDuino robot(1);

// ====== STATE MACHINE ======
enum State {
  WAITING,   // Waiting for banana plugs to be connected
  ACTIVE,    // Running for 40 seconds
  COOLDOWN   // Forced 3-minute shutdown
};
State currentState = WAITING;

// ====== TASK ACTIVATION VARIABLES ========
// THESE SHOULD ALL BE SET TO TRUE FOR SPRINTS AND COMPETITION 
bool centerLogged = false; // logging center reaching
bool defeatKoopaActive = true;
bool stabilizeReactorActive = true; // purely mechanical, this var does nothing
bool deliverMarioActive = true;
bool feedLumaActive = false;
const int DEFEAT_KOOPA_PIN = 2; // which DO the actuator is connected to.
const int PUSHBTN_PIN = 3; // which DI the push button is connected to.

// ====== TIMING VARIABLES ======
unsigned long stateStartTime = 0;
unsigned long motorStartTime = 0;
unsigned long activationCycleStarted = false;
bool motorStartSet = false;

const unsigned long ACTIVE_STATE_TIME = 40000;     // 40 seconds, reduce for comp or sprint for buffer
const unsigned long COOLDOWN_STATE_TIME = 180000;  // 3 minutes
const unsigned long KOOPA_LEAD_TIME = 5000;        // activate 5 seconds before hard timeout

// ====== 4-SECOND TIMER VARIABLES ======
bool triggered = false;
bool lastBtnState = false;
const unsigned long TIMEOUT_4S = 4000;
unsigned long timer4sStartTime = 0;
bool timer4sActive = false;

// ====== DELIVER MARIO TIMER VARIABLES ======
const unsigned long MARIO_RUNTIME = 10000; // 10 seconds, easily adjustable
bool marioRunning = false;
bool marioDone = false;
unsigned long marioStartTime = 0;

// ====== KOOPA TIMER VARIABLES ======
bool koopaTriggered = false;

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
  
  // Initialize the trigger button state
  lastBtnState = robot.readButton(PUSHBTN_PIN);

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
        if (!activationCycleStarted) {
          activationCycleStarted = true;
          stateStartTime = millis(); // Start 40s timer
        }
        
        // Start the 4-second button timer
        timer4sStartTime = millis();
        timer4sActive = true;
        triggered = false;

        // Reset deliver Mario state for a fresh run
        marioRunning = false;
        marioDone = false;
        marioStartTime = 0;

        // Reset Koopa trigger for a fresh run
        koopaTriggered = false;
        
        Serial.println("ACTIVE started.");
        if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
        }
        activateDriveTrain(motorStartTime);
      }

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME && activationCycleStarted) { // 40 seconds reached
        Serial.println("40s reached. Entering COOLDOWN.");
        deActivateSystem(); // Stop all systems regardless of plug state
        currentState = COOLDOWN;
        stateStartTime = millis(); // Start 3 min timer
      }
      break;

    case ACTIVE:
      if (bananaPlugState == 0) { // Banana Plugs disconnected prematurely
        Serial.println("Banana plugs disconnected.");
        deActivateSystem();
        
        // Stop/Reset the 4-second timer
        timer4sActive = false;
        triggered = false;

        // Reset deliver Mario state
        marioRunning = false;
        marioDone = false;
        marioStartTime = 0;

        // Reset Koopa trigger
        koopaTriggered = false;
        
        currentState = WAITING; // Return to waiting state
        break;
      }

      // --- 4-SECOND OVERRIDE LOGIC ---
      bool btnState = robot.readButton(PUSHBTN_PIN);
      
      // Check for button trigger
      if (!triggered && btnState != lastBtnState) {
        triggered = true;
      }
      lastBtnState = btnState;

      // Check if 4 seconds have passed in the ACTIVE state
      bool timeoutReached = timer4sActive && (millis() - timer4sStartTime >= TIMEOUT_4S);

      // Check if it is time to activate Koopa
      bool koopaTimeReached = activationCycleStarted &&
                              (millis() - stateStartTime >= ACTIVE_STATE_TIME - KOOPA_LEAD_TIME);

      if (!koopaTriggered && koopaTimeReached) {
        koopaTriggered = true;
        defeatKoopas();
        Serial.println("Koopa activated.");
      }

      if (triggered || timeoutReached) {
        // Hard override: Stop the big motor regardless of what activateDriveTrain wants
        robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);

        if (triggered && !centerLogged) {
          Serial.println("Center reached: button.");
          centerLogged = true;
        } else if (timeoutReached && !centerLogged) {
          Serial.println("Center reached: timeout.");
          centerLogged = true;
        }

        // robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0); Small motor is part of a different subsystem, DO NOT UNCOMMENT!
        robot.LED(1, false);

        // Deliver Mario subsystem
        deliverMario();
      } else {
        // Proceed with normal drive train logic if not triggered and under 4s
        if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
        }
        activateDriveTrain(motorStartTime); // Keep motors running while active
      }
      // --------------------------------

      // Feed Luma/Defeat Koopa Activation
      // Implementation TBD

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME && activationCycleStarted) { // 40 seconds reached
        Serial.println("Hard timeout reached. Entering COOLDOWN.");
        deActivateSystem(); // Stop all systems regardless of plug state
        
        // Stop/Reset the 4-second timer
        timer4sActive = false;
        triggered = false;

        // Reset deliver Mario state
        marioRunning = false;
        marioDone = false;
        marioStartTime = 0;

        // Reset Koopa trigger
        koopaTriggered = false;
        
        currentState = COOLDOWN;
        stateStartTime = millis(); // Start 3 min timer
      }
      break;

    case COOLDOWN:
      deActivateSystem(); // Enforce shutdown
      
      if (millis() - stateStartTime >= COOLDOWN_STATE_TIME) { // After 3 minutes
        currentState = WAITING;
        Serial.println("COOLDOWN complete.");
      }

      // reset motor active set flag
      motorStartSet = false;
      activationCycleStarted = false;
      // reset motor triggered flag
      triggered = false;

      // reset deliver Mario state
      marioRunning = false;
      marioDone = false;
      marioStartTime = 0;

      // reset Koopa trigger
      koopaTriggered = false;
      break;

    default:
      currentState = WAITING;
      break;
  }
  
  delay(50);
}

void activateDriveTrain(unsigned long motorStartTime) { // Activate System Subsystem
  int irDistanceRaw = robot.readIR();
  float irFiltered = convertIRReading(irDistanceRaw);
  
  // Motor runs continuously here. 
  // Stopping is handled strictly by the 4s timer/button override in loop().
  // Serial.println(irFiltered);
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED);
  robot.LED(1, true);
  
  // robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED); // DELETE IF NOT NEEDED
}

void deActivateSystem() { // Deactivate All Subsystems
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.LED(1, false);
  robot.digital(DEFEAT_KOOPA_PIN,0); // deactivates koopa piston;
}

float convertIRReading(int irValue) {
  float distance = 2076.0 / (irValue - 11.0);
  if (distance > 0 && distance < 150) { // Filter unreasonable values
    return distance;
  } else {
    return 99999; // return arbitrarily high so we don't accidentally stop
  }
}

// ===== IMPLEMENT LATER =====
void deliverMario() {
  if (!deliverMarioActive) {
    return;
  }

  if (!marioRunning && !marioDone) {
    marioRunning = true;
    marioStartTime = millis();
    robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 255);
    Serial.println("Deliver Mario started.");
    return;
  }

  if (marioRunning) {
    unsigned long marioElapsed = millis() - marioStartTime;

    if (marioElapsed >= MARIO_RUNTIME) {
      robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0);
      marioRunning = false;
      marioDone = true;
      Serial.println("Deliver Mario complete.");
    } else {
      robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 255);
    }
  }
}

void feedLumas() {}
void stabilizeReactor() {}

void defeatKoopas() {
  if (defeatKoopaActive) {
    robot.digital(DEFEAT_KOOPA_PIN,1);
  }
}