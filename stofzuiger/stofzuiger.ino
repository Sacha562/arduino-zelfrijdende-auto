#include <HCSR04.h>

enum States {IDLE, MOVING_FORWARD, MOVING_BACKWARD, TURNING_RIGHT, TURNING_LEFT};
enum Sides {IDK_MAN, RIGHT, LEFT};

const int triggerPinRight = 2;
const int echoPinRight = 4;
const int triggerPinLeft = 7;
const int echoPinLeft = 8;

const int minWallStreak = 3;

UltraSonicDistanceSensor distanceSensorRight(triggerPinRight, echoPinRight);
UltraSonicDistanceSensor distanceSensorLeft(triggerPinLeft, echoPinLeft);

int state = MOVING_FORWARD;
int wallStreakRight = 0;
int wallStreakLeft = 0;
double distanceRight = -1;
double distanceLeft = -1;

void setup() {
  Serial.begin(9600);
}

void loop() {
  distanceRight = distanceSensorRight.measureDistanceCm();
  distanceLeft = distanceSensorLeft.measureDistanceCm();

  //Serial.println(distanceRight);

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

  delay(50);
}

void standStill() {

}

void moveForward() {
  if (distanceRight >= 0 && distanceRight < 5) {
    wallStreakRight++;
  }
  else {
    wallStreakRight = 0;
  }

  if (distanceLeft >= 0 && distanceLeft < 5) {
    wallStreakLeft++;
  }
  else {
    wallStreakLeft = 0;
  }

  if (wallStreakRight >= minWallStreak || wallStreakLeft >= minWallStreak) {
    wallStreakRight = 0;
    wallStreakLeft = 0;
    hitWall();
  }
}

void moveBackward() {

}

void turnRight() {

}

void turnLeft() {

}

/////////////////////////////////////////////////////////

void hitWall() {
  Serial.println("Au, ik heb een muur geraakt. bEter ga ik draaien.");

  switch (getClosestSide()) {
    case RIGHT:
      Serial.println("Ik sla linksaf!");
      state = TURNING_LEFT;
      break;
    case LEFT:
      Serial.println("Ik sla rechtsaf!");
      state = TURNING_RIGHT;
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

int getClosestSide() {
  Serial.print("Aan de rechterkant heb ik nog ");
  Serial.print(distanceRight);
  Serial.print("cm over. Aan de linkerkant nog ");
  Serial.print(distanceLeft);
  Serial.println("cm.");
  
  if ((distanceRight == -1 && distanceLeft >= 0) || (distanceRight > distanceLeft)) return LEFT;
  else if (distanceLeft == -1 && distanceRight >= 0 || (distanceLeft > distanceRight)) return RIGHT;
  else return IDK_MAN;
}
