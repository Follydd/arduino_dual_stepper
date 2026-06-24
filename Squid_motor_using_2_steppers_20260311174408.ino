#include <AccelStepper.h>
#include <EnableInterrupt.h> // Library for PCI on any pin
#include <Servo.h>


// Define Pins x axis
#define STEP_PIN_X 7
#define DIR_PIN_X 6
#define HALL_RIGHT 10
#define HALL_LEFT 11
#define RIGHT true
#define LEFT false

// Define Pins y axis
#define STEP_PIN_Y 3
#define DIR_PIN_Y 2
#define HALL_UP 4
#define HALL_DOWN 5
#define UP true
#define DOWN false

// define laser and gun pins
#define LASER 9
#define GUN 8

#define X_SPEED 650
#define Y_SPEED 650

#define STEP 25

#define DBG(X) 

enum State {
  IDLE,
  ACTIVE,
  GOING_LEFT,
  GOING_RIGHT,
  DONE
};

Servo myservo;  // create servo object to control a servo

// Declare a variable of the enum type
State calibrateState = IDLE;

// Initialize Accelstepper_x (Driver mode = 1)
AccelStepper stepper_x(1, STEP_PIN_X, DIR_PIN_X);

// Initialize Accelstepper_y (Driver mode = 1)
AccelStepper stepper_y(1, STEP_PIN_Y, DIR_PIN_Y);

volatile bool stopRequested_x = false;
volatile bool direction_x = RIGHT;

volatile bool stopRequested_y = false;
volatile bool direction_y = UP;

int x_right_limit = 0;
int x_left_limit = 0;
int y_up_limit = 0;
int y_down_limit = 0;
int x_current;
int y_current;
static bool startup = true;
static bool centre = true;
void setup() {

  myservo.attach(GUN);  // attaches the servo on pin 9 to the servo object
  Serial.begin(115200);
  
  pinMode(HALL_RIGHT, INPUT_PULLUP);
  pinMode(HALL_LEFT, INPUT_PULLUP);
  pinMode(HALL_UP, INPUT_PULLUP);
  pinMode(HALL_DOWN, INPUT_PULLUP);

  pinMode(LASER,OUTPUT);
  pinMode(GUN,OUTPUT);

  // Enable Pin Change Interrupts on D10 and D11
  enableInterrupt(HALL_RIGHT, handleInterruptright, FALLING);
  enableInterrupt(HALL_LEFT, handleInterruptleft, FALLING);

  enableInterrupt(HALL_UP, handleInterruptup, FALLING);
  enableInterrupt(HALL_DOWN, handleInterruptdown, FALLING);

  stepper_x.setMaxSpeed(X_SPEED);
  stepper_x.setAcceleration(1000);
  
  stepper_y.setMaxSpeed(Y_SPEED);
  stepper_y.setAcceleration(800);

  DBG(Serial.println("System Ready. Send 'R' (Right) or 'L' (Left)");)
}

// ISR: This executes immediately when a magnet is detected
void handleInterruptright() {
  DBG(Serial.println("handleInterrupt right");)
  x_right_limit = stepper_x.currentPosition();
  if (direction_x == RIGHT)
    stopRequested_x = true;

  stopRequestX();
}

// ISR: This executes immediately when a magnet is detected
void handleInterruptleft() {
  DBG(Serial.println("handleInterrupt left");)
  x_left_limit = stepper_x.currentPosition();
  if (direction_x == LEFT)
    stopRequested_x = true;

  stopRequestX();
}

// ISR: This executes immediately when a magnet is detected
void handleInterruptup() {
  DBG(Serial.println("handleInterrupt up");)
  y_up_limit = stepper_y.currentPosition();
  if (direction_y == UP)
    stopRequested_y = true;

  stopRequestY();
}
// ISR: This executes immediately when a magnet is detected
void handleInterruptdown() {
  DBG(Serial.println("handleInterrupt down");)
  y_down_limit = stepper_y.currentPosition();
  if (direction_y == DOWN)
    stopRequested_y = true;

  stopRequestY();
}

void stepper_xmove(int x)
{
  stepper_x.move(x);
  x_current += x;
  
}
void stepper_ymove(int y)
{
  stepper_y.move(y);
  y_current += y;
  
}

void stopRequestX()
{
  // 2. Handle Interrupt Stop
  if (stopRequested_x) {
    //stepper_x.stop(); // Calculates deceleration stop
    // To stop INSTANTLY (not recommended at high speeds):
    stepper_x.setCurrentPosition(stepper_x.currentPosition());
    //stopRequested_x = false;
    DBG(Serial.println("LIMIT HIT X: Motor Halted.");)
    DBG(Serial.println(stepper_x.currentPosition());)

    if (direction_x == RIGHT)
      stepper_xmove(-60);
    if (direction_x == LEFT)
      stepper_xmove(60);
  }

}

void stopRequestY()
{
  // 2. Handle Interrupt Stop
  if (stopRequested_y) {
    //stepper_x.stop(); // Calculates deceleration stop
    // To stop INSTANTLY (not recommended at high speeds):
    stepper_y.setCurrentPosition(stepper_y.currentPosition());
    //stopRequested_y = false;
    DBG(Serial.println("LIMIT HIT Y: Motor Halted.");)
    DBG(Serial.println(stepper_y.currentPosition());)

    if (direction_y == UP)
      stepper_ymove(40);
    if (direction_y == DOWN)
      stepper_ymove(-40);
  }
}

long translateToSteps(int highLevelValue, long xlower, long xupper) {
  // 1. Constrain the input to your high-level range (0-1000)
  int clampedInput = constrain(highLevelValue, 0, 200);
  
  // 2. Map the 0-1000 range to your motor's step limits
  long targetStep = map(clampedInput, 0, 200, xlower, xupper);
  
  return targetStep;
}

void calibrate()
{
      delay(1000);

      DBG(Serial.println("In Startuo ...");)
      
      direction_x = RIGHT;
      stopRequested_x = false;
      DBG(Serial.println("Calibrate Moving Right...");)
      stepper_xmove(6000);
      while (stopRequested_x == false)
         stepper_x.run();
 
      direction_x = LEFT;
      stopRequested_x = false;
      stepper_xmove(-6000);
      DBG(Serial.println("Calibrate Moving Left...");)
      while (stopRequested_x == false)
       stepper_x.run();


      direction_y = UP;
      stopRequested_y = false;
      stepper_ymove(-2000);
      DBG(Serial.println("Calibrate Moving Up...");)
      while (stopRequested_y == false)
        stepper_y.run();

      
      direction_y = DOWN;
      stopRequested_y = false;
      stepper_ymove(2000);
      DBG(Serial.println("Calibrate Moving Down...");)
      while (stopRequested_y == false)
        stepper_y.run();
      
    startup = false;
}

void gotoCentre()
{
      stopRequested_y = false;
      stopRequested_x = false;
      
      int centre_x = ((x_right_limit - x_left_limit)/2) + x_left_limit;
      int centre_y = ((y_down_limit - y_up_limit)/2) + y_up_limit;

      stepper_y.moveTo(centre_y);
      stepper_x.moveTo(centre_x);
      centre = false;
      y_current = centre_y;
      x_current = centre_x;
      //Serial.println("INITIALISED and READY....");
}
void loop() {

   // 1. Process Serial Commands
  char cmd='M';
  if (Serial.available() > 0) {
    //char cmd = Serial.read();
    // 1. Read the incoming line until a newline character
    String input = Serial.readStringUntil('\n');
        // Variables to hold your data
    
    int input_x=0, input_y=0;

    // 2. Parse the mixed data using format specifiers:
    // %c = character, %d = decimal integer
    int parsed = sscanf(input.c_str(), "%c,%d,%d", &cmd, &input_x, &input_y);
    if (parsed < 1) {
      //Serial.println("ERROR INVALID COMMAND");
      return;
    }

    //goto certain position
    if (cmd == 'P') 
    {
      if (parsed < 3) {
        //Serial.println("ERROR BAD G FORMAT");
        return;
      }

      if (x_left_limit == 0 || x_right_limit == 0 || y_up_limit == 0 || y_down_limit == 0) {
        //Serial.println("ERROR UNINITIALISED");
      // This code runs if ANY of the four variables is 0
      }
      else
      {
        stopRequested_y = false;
        stopRequested_x = false;
        
        int current_x = stepper_x.currentPosition();
        int current_y = stepper_y.currentPosition();
        int required_x = translateToSteps(input_x,x_left_limit,x_right_limit);
        int required_y = translateToSteps(input_y,y_up_limit,y_down_limit);
        
        DBG(Serial.println("Current X" + String(current_x) + "Required X" + String(required_x));)
        if ((required_x - current_x) < 0)
        {
          direction_x = RIGHT;
          DBG(Serial.println("Moving Right...");)
        }
        else
        {
          direction_x = LEFT;
          DBG(Serial.println("Moving Left...");)
        }
        stopRequested_x = false;
        stepper_xmove(required_x-current_x);


        DBG(Serial.println("Current Y" + String(current_y) + "Required Y" + String(required_y));)
        if ((required_y - current_y) < 0)
        {
          direction_y = UP;
          DBG(Serial.println("Moving UP...");)
        }
        else
        {
          direction_y = DOWN;
          DBG(Serial.println("Moving Down...");)
        }
        stopRequested_y = false;
        stepper_ymove(required_y-current_y);

        //Serial.println("OK");
      }
    }
    
     //Laser on or OFF
    if (cmd == 'S') 
    {
      if (input_x == 1)
      {
        digitalWrite(LASER, HIGH); // Turn the LED on (sends 5V or 3.3V)
        delay(100); 
        //Serial.println("OK");
      }
      else
      {
        digitalWrite(LASER, LOW); // Turn the LED on (sends 5V or 3.3V)
        delay(100); 
        //Serial.println("OK");
      }
    }

    //Turn gun trigger on and OFF
    if (cmd == 'G') 
    {
      myservo.write(155); // Rotate CW
      delay(500);
      myservo.write(85);  // Stop
      delay(5000);
      //Serial.println("OK");
    }

    if (cmd == 'R') {
      
      direction_x = RIGHT;
      stopRequested_x = false;
      stepper_xmove(STEP);
      DBG(Serial.println("Moving Right...");)
    } else if (cmd == 'L') {
      
      direction_x = LEFT;
      stopRequested_x = false;
      stepper_xmove(-(STEP));
      DBG(Serial.println("Moving Left...");)
    }

    if (cmd == 'U') {
      
      direction_y = UP;
      stopRequested_y = false;
      stepper_ymove(-(STEP));
      DBG(Serial.println("Moving Up...");)
    } else if (cmd == 'D') {
      
      direction_y = DOWN;
      stopRequested_y = false;
      stepper_ymove(STEP);
      DBG(Serial.println("Moving Down...");)
    }

    if (cmd == 'C') {//calibrate

      calibrate();
    }
    if (cmd == 'T') {//calibrate

      gotoCentre();
    } 

      if (cmd == 'M') {//measure
      stopRequested_y = false;
      // Serial.println("x_right_limit");
      // Serial.println(x_right_limit);
      // Serial.println("x_left_limit");
      // Serial.println(x_left_limit);
      // Serial.println("y_up_limit");
      // Serial.println(y_up_limit);
      // Serial.println("y_down_limit");
      // Serial.println(y_down_limit);

        
      Serial.print(x_current);

      Serial.print(",");
      Serial.println(y_current);

    } 


  }

 // 3. Keep Motor Moving
  stepper_x.run();
  stepper_y.run();
}
