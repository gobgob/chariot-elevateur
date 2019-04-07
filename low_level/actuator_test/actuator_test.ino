#include <Dynamixel.h>
#include <DynamixelInterface.h>
#include <DynamixelMotor.h>

#define STEP 30
#define DIR 29
#define SLEEP 31
#define RESET 32
#define MS1 28
#define MS2 27
#define MS3 26

#define ENDSTOP 9

#define LED_ERROR 4
#define LED_WARN 5

#define VERSION "0.1.0"

#define ABSOLUTE 0
#define INCREMENTAL 1

char serialInput[10];
int serialPrompt = 0;

DynamixelInterface dInterface(Serial1);  // Stream
DynamixelMotor motorX(dInterface, 2);  // Interface, ID;
DynamixelMotor motorA(dInterface, 1); // Interface, ID;

int currentMotorXPosition;
int currentMotorAPosition;
float currentMotorYPosition;
int speed = 80;
int mode = ABSOLUTE;

void setup() {
  // AX-12
  dInterface.begin(1000000, 50); // baudrate, timeout

  motorX.init(); // This will get the returnStatusLevel of the servo
  Serial.printf("[Motor X] Status return level = %u\n", motorX.statusReturnLevel());
  motorX.jointMode(); // Set the angular limits of the servo. Set to [min, max] by default (85, 215)
  motorX.enableTorque();

  motorA.init(); // This will get the returnStatusLevel of the servo
  Serial.printf("[Motor A] Status return level = %u\n", motorA.statusReturnLevel());
  motorA.jointMode(); // Set the angular limits of the servo. Set to [min, max] by default (85, 215)
  motorA.enableTorque();

  // Steppers
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(SLEEP, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(MS3, OUTPUT);

  // wakeup
  digitalWrite(RESET, HIGH);
  digitalWrite(SLEEP, HIGH);

  // set microsteps to 16
  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);

  // Endstop
  pinMode(ENDSTOP, INPUT);

  // leds
  pinMode(LED_ERROR, OUTPUT);
  pinMode(LED_WARN, OUTPUT);

  help();
  init();
  ready();
}


void loop()
{
  readSerial();
}

/**
   Setup the Y
*/
void init() {
  digitalWrite(LED_WARN, HIGH);
  digitalWrite(LED_ERROR, HIGH);
  Serial.println("[MotorY] Calibration start");
  digitalWrite(DIR, HIGH);
  while (digitalRead(ENDSTOP) == LOW) {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(speed);
    digitalWrite(STEP, LOW);
    delayMicroseconds(speed);
  }
  currentMotorYPosition = 208; // Top
  Serial.println("[MotorY] Calibration finish");
  home();
  digitalWrite(LED_WARN, LOW);
}

/**
   Display helpful information
*/
void help() {
  Serial.print(F("Atome manipulator "));
  Serial.println(VERSION);
  Serial.println(F("Commands:"));
  Serial.println(F("G0 [X(steps)] [Y(steps)] [A(steps)] [F(feedrate)]; - linear move"));
  Serial.println(F("X(steps); - Move X axis"));
  Serial.println(F("Y(steps); - Move Y axis"));
  Serial.println(F("A(steps); - Move A axis"));
  Serial.println(F("G90; - absolute mode (default)"));
  Serial.println(F("G91; - incremental mode"));
  Serial.println(F("G28; - go home"));
  Serial.println(F("G29; - go middle"));
  Serial.println(F("M1; - init"));
  Serial.println(F("M100; - this help message"));
  Serial.println(F("M114; - report position"));
  Serial.println(F("M115; - get firmware version"));
}

/**
   Read serial inputs
*/
void readSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  serialInput[serialPrompt++] = c;

  if (c == '\n') {
    Serial.print(F("\n\r"));
    serialInput[serialPrompt] = 0; // strings must end with a \0
    processCommand();
    ready();
  }
}

/**
   Prepares the input buffer to receive a new message and
   tells the serial connected device it is ready for more.
*/
void ready() {
  serialPrompt = 0;
  Serial.print(F("> "));
}

void processCommand() {
  // look for command starting with "G"
  int cmd = parseNumber('G', -1);

  switch (cmd) {
    case 0: // move
      {
        int X = parseNumber('X', 0);
        int Y = parseNumber('Y', 0);
        int A = parseNumber('A', 0);
        int F = parseNumber('F', 0);

        if (F != 0) setFeedRate(F);
        if (A != 0) mode == INCREMENTAL ? motorAGotoInc(A) : motorAGotoAbs(A);
        if (X != 0) mode == INCREMENTAL ? motorXGotoInc(X) : motorXGotoAbs(X);
        if (Y != 0) mode == INCREMENTAL ? motorYGotoInc(Y) : motorYGotoAbs(Y);
        break;
      }

    case 28: home(); break;
    case 29: {
        setFeedRate(40);
        motorXGotoAbs(150);
        motorAGotoAbs(150);
        motorYGotoAbs(140);
        break;
      }

    case 90: mode = ABSOLUTE; break;
    case 91: mode = INCREMENTAL; break;
  }

  // look for command starting with "M"
  cmd = parseNumber('M', -1);

  switch (cmd) {
    case 1: init(); break;
    case 100: help(); break;
    case 114: reportPosition(); break;
    case 115:
      Serial.println("ok PROTOCOL_VERSION:0.0.1 FIRMWARE_NAME:AtomeManipulator");
      break;
  }

  // X axis shortcut
  cmd = parseNumber('X', 0);
  if (cmd != 0) mode == INCREMENTAL ? motorXGotoInc(cmd) : motorXGotoAbs(cmd);

  // Y axis shortcut
  cmd = parseNumber('Y', 0);
  if (cmd != 0) mode == INCREMENTAL ? motorYGotoInc(cmd) : motorYGotoAbs(cmd);

  // A axis shortcut
  cmd = parseNumber('A', 0);
  if (cmd != 0) mode == INCREMENTAL ? motorAGotoInc(cmd) : motorAGotoAbs(cmd);
}

/**
   Set Y speed, 40 is the faster
*/
void setFeedRate(int i) {
  if (i < 40 || i > 500) {
    Serial.println("[FeedRate] Range Error (40 to 500)");
  } else {
    speed = i;
  }
}

void motorAGotoAbs(int pos) {
  if (pos < 60 || pos > 190) {
    Serial.println("[Motor A] Range Error (60 to 190)");
  } else {
    Serial.print("[Motor X] goto: "); Serial.println(pos, DEC);
    motorA.goalPositionDegree(pos);
    currentMotorAPosition = pos;
  }
}

void motorAGotoInc(int pos) {
  motorAGotoAbs(currentMotorAPosition + pos);
}

void motorXGotoAbs(int pos) {
  if (pos < 85 || pos > 215) {
    Serial.println("[Motor X] Range Error (85 to 215)");
  } else {
    Serial.print("[Motor X] goto: "); Serial.println(pos, DEC);
    motorX.goalPositionDegree(pos);
    currentMotorXPosition = pos;
  }
}

void motorXGotoInc(int pos) {
  motorXGotoAbs(currentMotorXPosition + pos);
}

/**
   Move Y axis of `pos` mm (incremental)
*/
void motorYGotoInc(int pos) {
  if (currentMotorYPosition + pos < 0 || currentMotorYPosition + pos > 208) {
    Serial.println("[Motor Y] Range Error (0 to 208)");
  } else {
    // Direction
    digitalWrite(DIR, pos > 0 ? HIGH : LOW);

    float mmPerStep = 0.04 / 16; // 0.04 mm per step | 16 microsteps
    float goal = currentMotorYPosition + pos;

    while (fabs(currentMotorYPosition - goal) > 0.01) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(speed);
      digitalWrite(STEP, LOW);
      delayMicroseconds(speed);
      currentMotorYPosition += (digitalRead(DIR) == HIGH ? mmPerStep : -mmPerStep);
    }
  }
}

/**
   Move Y axis of `pos` mm (absolute)
*/
void motorYGotoAbs(int pos) {
  if (pos < 0 || pos > 205) {
    // Abs have less max value due to the endstop
    Serial.println("[Motor Y] Range Error (0 to 205)");
  } else {
    // Direction
    digitalWrite(DIR, pos - currentMotorYPosition > 0 ? HIGH : LOW);

    float mmPerStep = 0.04 / 16; // 0.04 mm per step | 16 microsteps

    while (fabs(currentMotorYPosition - pos) > 0.01) {
      if (digitalRead(ENDSTOP) == HIGH) break;
      digitalWrite(STEP, HIGH);
      delayMicroseconds(speed);
      digitalWrite(STEP, LOW);
      delayMicroseconds(speed);
      currentMotorYPosition += (digitalRead(DIR) == HIGH ? mmPerStep : -mmPerStep);
    }
  }

}

void reportPosition() {
  Serial.print("[Motor X]: position = "); Serial.println(currentMotorXPosition, DEC);
  Serial.print("[Motor Y]: position = "); Serial.println(currentMotorYPosition, DEC);
  Serial.print("[Motor A]: position = "); Serial.println(currentMotorAPosition, DEC);
  Serial.print("[FeedRate]: speed = "); Serial.println(speed, DEC);
}

void home() {
  Serial.println("Go home");
  setFeedRate(40);
  digitalRead(ENDSTOP) == HIGH ? motorYGotoInc(-20) : motorYGotoAbs(208 - 20);
  motorXGotoAbs(150);
  motorAGotoAbs(60);
}

/**
   Look for character /code/ in the buffer and read the float that immediately follows it.
   @return the value found.  If nothing is found, /val/ is returned.
   @input code the character to look for.
   @input val the return value if /code/ is not found.
 **/
float parseNumber(char code, float val) {
  char *ptr = serialInput; // start at the beginning of buffer
  while ((long)ptr > 1 && (*ptr) && (long)ptr < (long)serialInput + serialPrompt) { // walk to the end
    if (*ptr == code) { // if you find code on your walk,
      return atof(ptr + 1); // convert the digits that follow into a float and return it
    }
    ptr = strchr(ptr, ' ') + 1; // take a step from here to the letter after the next space
  }
  return val;  // end reached, nothing found, return default val.
}