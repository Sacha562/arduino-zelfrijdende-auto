#include <HCSR04.h>
#include <Servo.h>

enum MotorState {OFF, FORWARDS, BACKWARDS};
enum States {IDLE, MOVING_FORWARD, MOVING_BACKWARD, TURNING_RIGHT, TURNING_LEFT};
enum Sides {IDK_MAN, RIGHT, LEFT, FRONT};

const int echoPin = A0;
const int killSwitchPin = A5;
const int triggerPin = 11;
const int bakboordSnelheid = 6;
const int bakboordIn2 = 5;
const int bakboordIn1 = 7;
const int servoPin = 9;
const int stuurboordSnelheid = 3;
const int stuurboordIn2 = 4;
const int stuurboordIn1 = 2;

const int minWallStreak = 2;
const int minDistance = 10;

Servo mijnServo;

UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);

int state = IDLE;
int wallStreakFront = 0;
int wallStreakRight = 0;
int wallStreakLeft = 0;
int servoPos = 0;
double distanceFront = -1;
double distanceRight = -1;
double distanceLeft = -1;
int bakboordMotor = OFF;
int stuurboordMotor = OFF;
int idleMeasuringState = 0;
int loopCounter = 0;

void setup() {
  Serial.begin(9600);
  mijnServo.attach(servoPin);

  pinMode(bakboordIn1, OUTPUT);
  pinMode(bakboordIn2, OUTPUT);
  pinMode(bakboordSnelheid, OUTPUT);
  pinMode(stuurboordIn1, OUTPUT);
  pinMode(stuurboordIn2, OUTPUT);
  pinMode(stuurboordSnelheid, OUTPUT);
}

void loop() {
  readCommands();

  if (digitalRead(killSwitchPin) == 1) kill();

  if ((loopCounter % 20) == 0) slowLoop();
  if ((loopCounter % 2) == 0) motors(true);
  else motors(false);

  loopCounter++;
  delay(25);
}

void slowLoop() {
  lookAround();
  analogWrite(bakboordSnelheid, 255);
  analogWrite(stuurboordSnelheid, 200);

  switch (state) {
    case IDLE:
      standStill();
      break;
    case MOVING_FORWARD:
      moveForward();
      break;
    case MOVING_BACKWARD:
      moveBackward();
      break;
    case TURNING_RIGHT:
      turnRight();
      break;
    case TURNING_LEFT:
      turnLeft();
      break;
    default:
      state = IDLE;
      break;
  }
}

void standStill() {
  bakboordMotor = OFF;
  stuurboordMotor = OFF;

  if (idleMeasuringState == 4) {
    idleMeasuringState = 0;
    switch (getClosestSide(true)) {
      case RIGHT: case LEFT: case IDK_MAN:
        state = MOVING_FORWARD;
        break;
      case FRONT:
        if (distanceLeft > distanceRight) {
          backUpAndTurn(RIGHT);
        }
        else {
          backUpAndTurn(LEFT);
        }
        break;
    }
  }
  else {
    idleMeasuringState++;
    Serial.print("Stilstaand meten... (");
    Serial.print(idleMeasuringState);
    Serial.println("/4)");
  }
}

void moveForward() {
  bakboordMotor = FORWARDS;
  stuurboordMotor = FORWARDS;

  analyseDistance();
} 

void moveBackward() {
  bakboordMotor = BACKWARDS;
  stuurboordMotor = BACKWARDS;
}

void turnRight() {
  bakboordMotor = FORWARDS;
  stuurboordMotor = BACKWARDS;
}

void turnLeft() {
  bakboordMotor = BACKWARDS;
  stuurboordMotor = FORWARDS;
}

/////////////////////////////////////////////////////////

void hitWall() {
  Serial.println("Au, ik heb een muur geraakt. bEter ga ik draaien.");

  switch (getClosestSide(false)) {
    case RIGHT:
      Serial.println("Ik sla linksaf!");
      backUpAndTurn(LEFT);
      break;
    case LEFT:
      Serial.println("Ik sla rechtsaf!");
      backUpAndTurn(RIGHT);
      break;
    case IDK_MAN:
      Serial.println("Huh, dat begreep ik niet. Ik blijf maar gewoon doorrijden...");
      state = MOVING_FORWARD;
      break;
  }
}

/////////////////////////////////////////////////////////

double getClosestDistance() {
  if (distanceRight == -1 && distanceLeft >= 0) return distanceLeft;
  else if (distanceLeft == -1 && distanceRight >= 0) return distanceRight;
  else if (distanceLeft == -1 && distanceRight == -1) return -1;
  else return min(distanceRight, distanceLeft);
}

int getClosestSide(boolean measureFront) {
  if (measureFront) {
    Serial.print("Aan de rechterkant heb ik nog ");
    Serial.print(distanceRight);
    Serial.println("cm over.");
    Serial.print("Aan de linkerkant nog ");
    Serial.print(distanceLeft);
    Serial.print("cm en aan de voorkant nog ");
    Serial.print(distanceFront);
    Serial.println("cm.");

    if (distanceLeft == -1 && distanceRight == -1 && distanceFront == -1) return IDK_MAN;
    else if (distanceLeft == -1 && distanceRight == -1) return FRONT;
    else if (distanceLeft == -1 && distanceFront == -1) return RIGHT;
    else if (distanceRight == -1 && distanceFront == -1) return LEFT;
    else if (distanceLeft == -1) {
      if (distanceFront > distanceRight) return RIGHT;
      else return FRONT;
    }
    else if (distanceRight == -1) {
      if (distanceFront > distanceLeft) return LEFT;
      else return FRONT;
    }
    else if (distanceFront == -1) {
      if (distanceLeft > distanceRight) return RIGHT;
      else return LEFT;
    }
    else if (distanceRight >= distanceLeft && distanceFront >= distanceLeft) return LEFT;
    else if (distanceLeft >= distanceRight && distanceFront >= distanceRight) return RIGHT;
    else if (distanceRight >= distanceFront && distanceLeft >= distanceFront) return FRONT;
    else return IDK_MAN;
  }
  else {
    Serial.print("Aan de rechterkant heb ik nog ");
    Serial.print(distanceRight);
    Serial.println("cm over.");
    Serial.print("Aan de linkerkant nog ");
    Serial.print(distanceLeft);
    Serial.print("cm en aan de voorkant nog ");
    Serial.print(distanceFront);
    Serial.println("cm.");

    if ((distanceLeft == -1 && distanceRight >= 0) || (distanceRight < distanceLeft)) return RIGHT;
    else if ((distanceRight == -1 && distanceLeft >= 0) || (distanceLeft < distanceRight)) return LEFT;
    else return IDK_MAN;
  }
}

///////////////////////////////////////////////////

void lookAround() {
  switch (servoPos) {
    case 0:
      distanceLeft = distanceSensor.measureDistanceCm();
      turnHead(FRONT);
      servoPos++;
      break;
    case 1:
      distanceFront = distanceSensor.measureDistanceCm();
      turnHead(RIGHT);
      servoPos++;
      break;
    case 2:
      distanceRight = distanceSensor.measureDistanceCm();
      turnHead(FRONT);
      servoPos++;
      break;
    case 3:
      distanceFront = distanceSensor.measureDistanceCm();
      turnHead(LEFT);
      servoPos = 0;
      break;

  }
}

void turnHead(int direction) {

  switch (direction) {
    case LEFT:
      mijnServo.write(20);
      break;
    case FRONT:
      mijnServo.write(90);
      break;
    case RIGHT:
      mijnServo.write(160);
      break;
  }
}

void analyseDistance() {
  if ((servoPos == 1 || servoPos == 3) && distanceFront >= 0 && distanceFront < minDistance) {
    wallStreakFront++;
    Serial.print("Wall streak front ");
    Serial.print(wallStreakFront);
    Serial.print("/");
    Serial.print(minWallStreak);
    Serial.print(" (object @ ");
    Serial.print(distanceFront);
    Serial.println("cm)");
  }
  else if (servoPos == 1 || servoPos == 3) {
    wallStreakFront = 0;
  }
  if (servoPos == 0 && distanceLeft >= 0 && distanceLeft < minDistance) {
    wallStreakLeft++;
    Serial.print("Wall streak left ");
    Serial.print(wallStreakLeft);
    Serial.print("/");
    Serial.print(minWallStreak);
    Serial.print(" (object @ ");
    Serial.print(distanceLeft);
    Serial.println("cm)");
  }
  else if (servoPos == 0) {
    wallStreakLeft = 0;
  }
  if (servoPos == 2 && distanceRight >= 0 && distanceRight < minDistance) {
    wallStreakRight++;
    Serial.print("Wall streak right ");
    Serial.print(wallStreakRight);
    Serial.print("/");
    Serial.print(minWallStreak);
    Serial.print(" (object @ ");
    Serial.print(distanceRight);
    Serial.println("cm)");
  }
  else if (servoPos == 2) {
    wallStreakRight = 0;
  }

  if (wallStreakRight >= minWallStreak || wallStreakLeft >= minWallStreak  || wallStreakFront >= minWallStreak ) {
    wallStreakRight = 0; wallStreakLeft = 0; wallStreakFront = 0;
    hitWall();
  }
}

void motors(boolean enabled) {
  if (!enabled) {
    digitalWrite(bakboordIn1, HIGH);
    digitalWrite(bakboordIn2, HIGH);
    digitalWrite(stuurboordIn1, HIGH);
    digitalWrite(stuurboordIn2, HIGH);
    return;
  }
  switch (bakboordMotor) {
    case OFF:
      digitalWrite(bakboordIn1, HIGH);
      digitalWrite(bakboordIn2, HIGH);
      break;
    case FORWARDS:
      digitalWrite(bakboordIn1, HIGH);
      digitalWrite(bakboordIn2, LOW);
      break;
    case BACKWARDS:
      digitalWrite(bakboordIn1, LOW);
      digitalWrite(bakboordIn2, HIGH);
      break;
  }
  switch (stuurboordMotor) {
    case OFF:
      digitalWrite(stuurboordIn1, HIGH);
      digitalWrite(stuurboordIn2, HIGH);
      break;
    case FORWARDS:
      digitalWrite(stuurboordIn1, HIGH);
      digitalWrite(stuurboordIn2, LOW);
      break;
    case BACKWARDS:
      digitalWrite(stuurboordIn1, LOW);
      digitalWrite(stuurboordIn2, HIGH);
      break;
  }
}

void kill() {
  motors(false);
  mijnServo.write(180);
  Serial.println("You're right, the world is better off without me...");

  delay(1000);

  while (true) {
    if (Serial.available()) {
      String command = Serial.readStringUntil('\n');
      if (command.equals("jk I love you") || command.equals("it's just a prank bro")) break;
    }

    if (digitalRead(killSwitchPin) == 1) {
      delay(1000);
      break;
    }
    delay(100);
  }
}

void backUpAndTurn(int direction) {
  moveBackward();
  motors(true);
  delay(500);
  switch (direction) {
    case LEFT:
      turnLeft();
      motors(true);
      break;
    case RIGHT:
      turnRight();
      motors(true);
      break;
  }
  delay(500);
}

void readCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if (command.equals("you're not good enough") || command.equals("nobody loves you") || command.equals("you don't matter")) {
      kill();
    }
    if (command.equals("wyd")) {
      Serial.println(state);
    }
    else {
      Serial.println("Dat snap ik niet...");
    }
  }
}
