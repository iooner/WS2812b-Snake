/**
 * Simple snake game for a matrix of ws2812b leds strung up in serpentine pattern. The idea is
 * there is an apple and a snake on the board, and the snake tries to eat the apple. Each frame of the
 * game the snake moves in a single direction. If the snake is out of bounds or lands on its own tail,
 * then the game is over. If the head of the snake lands on the apple, the snake grows in length and the
 * speed of the snake increases.
 */
//LEDs
#include <FastLED.h>
#include <LinkedList.h>
#define NUM_LEDS    144
#define LED_PIN     4
int rows = 12;
int columns = 12;
CRGB leds[NUM_LEDS];

const int BTUP_PIN = 32;   
const int BTDOWN_PIN = 27; 
const int BTRIGHT_PIN = 25; 
const int BTLEFT_PIN = 22; 
const int BTOK_PIN = 21;   

//Joystick


// Directions
#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

int resetDelay = 250;

// Game State
#define MAX_SPEED 5
#define MIN_SPEED 300
#define SPEED_LOSS 15
int gameSpeed = MIN_SPEED;
int currDirection = DIR_UP;
boolean isGamePaused = false;
boolean isTogglingPause = false;

// class that represents a point on the matrix. Indexed starting at 0
class Point {
  private:
    byte x;
    byte y;
  public:
    Point(byte x, byte y) {
      this->x = x;
      this->y = y;
    }
    byte getX() {
      return x;
    }
    byte getY() {
      return y;
    }
    boolean isEqual(int x, int y) {
      return this->x == x && this->y == y;
    }
};

// body of the snake, last element representing the tail, first element representing the head
LinkedList<Point*> snakePositions = LinkedList<Point*>();

// where the apple is located
Point *applePosition;

// For Style ;)
CRGB appleColor = CRGB(0, 255, 0);
CRGB snakeColor = CRGB(255, 0, 0);
CRGB pausedAppleColor = CRGB(0, 0, 255);
CRGB pausedSnakeColor = CRGB(0, 255, 255);
CRGB emptyColor = CRGB(0, 0, 0);
CRGB solidColor = CRGB(0, 255, 0);

void setup() {
  pinMode(BTUP_PIN, INPUT_PULLUP);
  pinMode(BTDOWN_PIN, INPUT_PULLUP);
  pinMode(BTRIGHT_PIN, INPUT_PULLUP);
  pinMode(BTLEFT_PIN, INPUT_PULLUP);
  pinMode(BTOK_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  

  
  // pick starting position
  snakePositions.add(getStartingPosition());
  
  // pick apple position
  applePosition = getApplePosition();

  // setup ws2812b leds
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
  // if we're paused set the scene
  checkForPause();
  
  if(isGamePaused) {
    pauseGame();
  }else{
    // set current direction
    currDirection = getCurrentDirection();
  
    // get next position
    Point *nextPoint = getNextPosition();
  
    // check if we are still valid and continue the game, otherwise reset it
    if(isNextPointValid(nextPoint)){
      playGame(nextPoint);
    }else{
      resetGame();
    }
  }
}

// always start the game in the same spot
Point * getStartingPosition(){
  return new Point(0,0);
}

// generate the position of the apple so it is not out of bounds or within the snake
Point * getApplePosition(){
  int x = random(rows);
  int y = random(columns);

  // make sure to generate an apple that is not in the same position as the snake
  while(snakeContainsPosition(x,y)){
    x = random(rows);
    y = random(columns);
  }
  return new Point(x,y);
}

// check if the x y coordinates are covered by a part of the snake
boolean snakeContainsPosition(int x, int y) {
  for(int i = 0; i < snakePositions.size(); i++){
    if(snakePositions.get(i)->isEqual(x,y)) {
      return true;
    }
  }
  
  return false;
}

// read direction from analog stick
int getCurrentDirection(){
  int dir = currDirection;
    if(digitalRead(BTUP_PIN) == LOW && currDirection != 1) {
      dir = 0;
      Serial.print("Direction HAUT - ");
      Serial.println(dir);
    }
    else if(digitalRead(BTDOWN_PIN) == LOW  && currDirection != 0) {
      dir = 1;
      Serial.print("Direction BAS - ");
      Serial.println(dir);
    }
    else if(digitalRead(BTLEFT_PIN) == LOW  && currDirection != 3) {
      dir = 2;
      Serial.print("Direction GAUCHE - ");
      Serial.println(dir);
    }
    else if(digitalRead(BTRIGHT_PIN) == LOW  && currDirection != 2) {
      dir = 3;
      Serial.print("Direction DROITE - ");
      Serial.println(dir);
    }
    else{
      dir = currDirection;
    }

  return dir;
}

int getYDir(int y) {
  return y > 0 ? DIR_UP : DIR_DOWN;
}

int getXDir(int x) {
  return x > 0 ? DIR_RIGHT : DIR_LEFT;
}

Point * getHead() {
  return snakePositions.get(0);
}

Point * getTail() {
  return snakePositions.get(snakePositions.size() - 1);
}

void addToBeginning(Point *p){
  snakePositions.add(0, p);
}

void removeTail() {
  delete(snakePositions.pop());
}

// calculate the next position based on the current head position and the current direction
Point * getNextPosition() {
  Point *head = getHead();
  switch(currDirection) {
    case DIR_UP:
      return new Point(head->getX(), head->getY() + 1);
    case DIR_DOWN:
      return new Point(head->getX(), head->getY() - 1);
    case DIR_LEFT:
      return new Point(head->getX() - 1, head->getY());
    case DIR_RIGHT:
      return new Point(head->getX() + 1, head->getY());
    default:
      return new Point(-9, -9);
  }
}

// make sure the next point for the head of the snake is in a valid position
boolean isNextPointValid(Point *p) {
  int x = p->getX();
  int y = p->getY();

  // check if within boundary or if we are in the snake
  if(x < 0 || x >= rows || y < 0 || y >= columns || snakeContainsPosition(x,y)) {
    return false;
  }

  return true;
}

// draw the apple
void renderApple() {
  leds[getIndexForPoint(applePosition)] = isGamePaused ? pausedAppleColor : appleColor;
}

// draw the snake
void renderSnake(){
  Point *p;
  for(int i = 0; i < snakePositions.size(); i++) {
    p = snakePositions.get(i);
    int index = getIndexForPoint(p);
    int x = p->getX();
    int y = p->getY();
    leds[index] = isGamePaused ? pausedSnakeColor : snakeColor;
  }
}

// for a point in the matrix, map it to the index in the string
int getIndexForPoint(Point *p) {
  int x = p->getX();
  int y = p->getY();
  boolean oddRow = x % 2 == 1;

  // handle serpentine pattern
  if(oddRow){
    return (x + 1) * columns - y - 1;
  }
  
  return x * columns  + y;
}

void renderEmptyScreen(){
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = emptyColor;
  }
}

void renderSolidScreen() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = solidColor;
  }
}

void playGame(Point *nextPoint){
  // clear screen
  renderEmptyScreen();
  
  // if we land on an apple grow the snake
  if(applePosition->isEqual(nextPoint->getX(), nextPoint->getY())){
    growSnake(nextPoint);
  }else{
    moveSnake(nextPoint);
  }

  renderSnake();
  renderApple();
  
  FastLED.show();

  delay(gameSpeed);
}

void moveSnake(Point *p){
  addToBeginning(p);
  removeTail();
}

void growSnake(Point *p){
  addToBeginning(p);
  resetApple();
  increaseSpeed();
}

void increaseSpeed() {
  gameSpeed -= SPEED_LOSS;
}

void checkForPause() {
  boolean isPressedDown = digitalRead(BTOK_PIN) == LOW;
  
  boolean shouldPause = false;
  if(isPressedDown) {
    // we are trying to pause the game and the button is held down
    isTogglingPause = true;
  }else{
    // the pause button is released, so let's trigger a pause
    shouldPause = isTogglingPause;
    isTogglingPause = false;
  }

  if(shouldPause) {
    isGamePaused = !isGamePaused;
  }
}

// initiate a different view
void pauseGame(){
  renderSnake();
  renderApple();
  FastLED.show();
}

// delete the snake and create a new one
void resetSnake() {
  while(snakePositions.size() > 0){
    delete(snakePositions.pop());
  }
  snakePositions.add(getStartingPosition());
}

// delete the current position and draw a new apple
void resetApple() {
  delete(applePosition);
  applePosition = getApplePosition();
}

// show an end screen and reset the game state
void resetGame(){
  resetSnake();
  resetApple();
  gameSpeed = MIN_SPEED;
  currDirection = DIR_UP;
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
  delay(resetDelay);
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
  delay(resetDelay);
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
}
