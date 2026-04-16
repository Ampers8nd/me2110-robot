#include <myDuino.h>
myDuino robot(1);

// ====== STATE MACHINE ======
enum State {
  WAITING,   // Waiting for banana plugs to be connected
  ACTIVE,    // Running for 40 seconds
  COOLDOWN   // Forced 3-minute shutdown
};
State currentState = WAITING;

// ====== PUSHBUTTON CONTROL ======
bool usePushbuttonStop = true; 
bool centerLogged = false;

// ====== TASK ACTIVATION VARIABLES ========
bool defeatKoopaActive = false;
bool stabilizeReactorActive = true;
bool deliverMarioActive = false;
bool feedLumaActive = false;
const int DEFEAT_KOOPA_PIN = 2;
const int PUSHBTN_PIN = 3; 

// ====== TIMING VARIABLES ======
unsigned long stateStartTime = 0;
unsigned long motorStartTime = 0;
unsigned long activationCycleStarted = false;
bool motorStartSet = false;

const unsigned long ACTIVE_STATE_TIME = 40000;
const unsigned long COOLDOWN_STATE_TIME = 180000;
const unsigned long KOOPA_LEAD_TIME = 5000;

// ====== DRIVETRAIN TIMER VARIABLES ======
const unsigned long TIMEOUT_4S = 4000;
unsigned long timer4sStartTime = 0;
bool timer4sActive = false;

// ====== PUSHBUTTON VARIABLES ======
bool btnTriggered = false;
bool lastBtnState = false;

// ====== DELIVER MARIO TIMER VARIABLES ======
const unsigned long MARIO_DELAY = 1000;   // adjustable delay after stopping before Mario starts
const unsigned long MARIO_RUNTIME = 1600; // adjustable Mario runtime
bool marioWaiting = false;
bool marioRunning = false;
bool marioDone = false;
unsigned long marioDelayStartTime = 0;
unsigned long marioStartTime = 0;

// ====== KOOPA TIMER VARIABLES ======
bool koopaTriggered = false;

// ===== MOTOR VARIABLES =====
const int BIG_MOTOR_PIN = 1;
const int SMALL_MOTOR_PIN = 2;
const int MOTOR_SPEED = 200;
const int MOTOR_FORWARD = 1;
const int MOTOR_BACKWARD = 2;

// ====== BUTTON DEBOUNCE ======
const int BANANA_PLUG_PIN = 1;
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

  lastBtnState = robot.readButton(PUSHBTN_PIN);

  attachInterrupt(digitalPinToInterrupt(2), encHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), encHandler, CHANGE);
}

void loop() {
  int bananaPlugReading = robot.readButton(BANANA_PLUG_PIN);
  if (bananaPlugReading != lastBananaPlugReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    bananaPlugState = bananaPlugReading;
  }
  lastBananaPlugReading = bananaPlugReading;

  switch (currentState) {

    case WAITING:
      deActivateSystem();

      if (bananaPlugState == 1) {
        currentState = ACTIVE;
        if (!activationCycleStarted) {
          activationCycleStarted = true;
          stateStartTime = millis();
        }

        timer4sStartTime = millis();
        timer4sActive = true;

        btnTriggered = false;
        centerLogged = false;

        marioWaiting = false;
        marioRunning = false;
        marioDone = false;
        marioDelayStartTime = 0;
        marioStartTime = 0;

        koopaTriggered = false;

        Serial.println("ACTIVE started.");
        if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
        }
        activateDriveTrain(motorStartTime);
      }

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME && activationCycleStarted) {
        Serial.println("40s reached. Entering COOLDOWN.");
        deActivateSystem();
        currentState = COOLDOWN;
        stateStartTime = millis();
      }
      break;

    case ACTIVE:
      if (bananaPlugState == 0) {
        Serial.println("Banana plugs disconnected.");
        deActivateSystem();

        timer4sActive = false;
        btnTriggered = false;
        centerLogged = false;

        marioWaiting = false;
        marioRunning = false;
        marioDone = false;
        marioDelayStartTime = 0;
        marioStartTime = 0;

        koopaTriggered = false;

        currentState = WAITING;
        break;
      }

      bool btnState = robot.readButton(PUSHBTN_PIN);
      if (!btnTriggered && btnState != lastBtnState) {
        btnTriggered = true;
      }
      lastBtnState = btnState;

      bool timeoutReached = timer4sActive && (millis() - timer4sStartTime >= TIMEOUT_4S);

      bool stopConditionMet = false;
      if (usePushbuttonStop) {
        stopConditionMet = btnTriggered || timeoutReached;
      } else {
        stopConditionMet = timeoutReached;
      }

      bool koopaTimeReached = activationCycleStarted &&
                              (millis() - stateStartTime >= ACTIVE_STATE_TIME - KOOPA_LEAD_TIME);

      if (!koopaTriggered && koopaTimeReached) {
        koopaTriggered = true;
        defeatKoopas();
        Serial.println("Koopa activated (5s before hard timeout).");
      }

      if (stopConditionMet) {
        robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);

        if (!centerLogged) {
          if (btnTriggered && !timeoutReached && usePushbuttonStop) {
            Serial.println("Center reached: pushbutton.");
          } else {
            Serial.println("Center reached: timer.");
          }
          centerLogged = true;
        }

        robot.LED(1, false);

        deliverMario();
        
      } else {
        if (!motorStartSet) {
          motorStartTime = millis();
          motorStartSet = true;
        }
        activateDriveTrain(motorStartTime);
      }

      if (millis() - stateStartTime >= ACTIVE_STATE_TIME && activationCycleStarted) {
        Serial.println("Hard timeout reached. Entering COOLDOWN.");
        deActivateSystem();

        timer4sActive = false;
        btnTriggered = false;
        centerLogged = false;

        marioWaiting = false;
        marioRunning = false;
        marioDone = false;
        marioDelayStartTime = 0;
        marioStartTime = 0;

        koopaTriggered = false;

        currentState = COOLDOWN;
        stateStartTime = millis();
      }
      break;

    case COOLDOWN:
      deActivateSystem();

      if (millis() - stateStartTime >= COOLDOWN_STATE_TIME) {
        currentState = WAITING;
        Serial.println("COOLDOWN complete.");
      }

      motorStartSet = false;
      activationCycleStarted = false;

      timer4sActive = false;
      btnTriggered = false;
      centerLogged = false;

      marioWaiting = false;
      marioRunning = false;
      marioDone = false;
      marioDelayStartTime = 0;
      marioStartTime = 0;

      koopaTriggered = false;
      break;

    default:
      currentState = WAITING;
      break;
  }

  delay(50);
}

void activateDriveTrain(unsigned long motorStartTime) {
  int irDistanceRaw = robot.readIR();
  float irFiltered = convertIRReading(irDistanceRaw);

  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, MOTOR_SPEED);
  robot.LED(1, true);
}

void deActivateSystem() {
  robot.moveMotor(BIG_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0);
  robot.LED(1, false);
  robot.digital(DEFEAT_KOOPA_PIN, 0);
}

float convertIRReading(int irValue) {
  float distance = 2076.0 / (irValue - 11.0);
  if (distance > 0 && distance < 150) {
    return distance;
  } else {
    return 99999;
  }
}

void deliverMario() {
  if (!deliverMarioActive) {
    return;
  }

  // First call after stopping: begin delay period
  if (!marioWaiting && !marioRunning && !marioDone) {
    marioWaiting = true;
    marioDelayStartTime = millis();
    Serial.println("Deliver Mario delay started.");
    return;
  }

  // Wait adjustable delay before starting Mario motor
  if (marioWaiting && !marioRunning) {
    if (millis() - marioDelayStartTime >= MARIO_DELAY) {
      marioWaiting = false;
      marioRunning = true;
      marioStartTime = millis();
      robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 255);
      Serial.println("Deliver Mario started.");
    }
    return;
  }

  // Run Mario motor for adjustable runtime
  if (marioRunning) {
    unsigned long marioElapsed = millis() - marioStartTime;

    if (marioElapsed >= MARIO_RUNTIME) {
      robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 0);
      marioRunning = false;
      marioDone = true;
      Serial.println("Deliver Mario complete.");
    } else {
      robot.moveMotor(SMALL_MOTOR_PIN, MOTOR_FORWARD, 155);
    }
  }
}

void feedLumas() {}
void stabilizeReactor() {}

void defeatKoopas() {
  if (defeatKoopaActive) {
    robot.digital(DEFEAT_KOOPA_PIN, 1);
  }
}