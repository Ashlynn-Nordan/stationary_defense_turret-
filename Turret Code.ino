#include <Servo.h>// Include the standard library to control servo motors

Servo latchServo;// Create a servo object to control the launcher release latch
Servo ballServo;// Create a servo object to control the ball feeder mechanism

int t = 8;// Delay (ms) for winding up the launcher string
int tf = 5;// Delay (ms) for loosening the string (unwinding)
int tUD = 4;// Delay (ms) for the vertical stepper motor
int tl = 2; // Delay (ms) for the horizontal rotation stepper

int triggerSwitch = 7;// Pin for the physical button that fires the launcher
int leftbutton = 18;// Interrupt pin for the manual "Rotate Left" button
int rightbutton = 19;// Interrupt pin for the manual "Rotate Right" button
int upbutton = 20;// Interrupt pin for the manual "Tilt Up" button
int modePin = 6; // Pin for the jumper for Manual vs Auto mode
int readyLED = 13;// Pin for the LED that lights up when the launcher is ready

// Ultrasonic sensor pins
int trigFront = 40;// Output pin for the front ultrasonic sensor trigger
int echoFront = 41;// Input pin for the front ultrasonic sensor echo
int trigLeft = 42;// Output pin for the left ultrasonic sensor trigger
int echoLeft = 43;// Input pin for the left ultrasonic sensor echo
int trigRight = 44;// Output pin for the right ultrasonic sensor trigger
int echoRight = 45;// Input pin for the right ultrasonic sensor echo

// Variables to store distance readings from sensors
long distanceFront;// Calculated distance from the front sensor
long distanceLeft;// Calculated distance from the left sensor
long distanceRight;// Calculated distance from the right sensor

// Flags for interrupts (volatile because they change inside ISR functions)
volatile bool leftRequest = false;  // Flag set true when the left button is pressed
volatile bool rightRequest = false; // Flag set true when the right button is pressed
volatile bool upRequest = false;    // Flag set true when the up button is pressed

// Step counters to manage stepper motor movement
int turretStepsLeft = 0;// Number of steps remaining to move left
int turretStepsRight = 0;// Number of steps remaining to move right
int turretStepsUp = 0;// Number of steps remaining to tilt up
int turretStepsDown = 0;// Number of steps remaining to tilt down

// Position trackers for "Return to Center" functionality
int turretPosition = 0;// Net horizontal position (positive = right, negative = left)
int turretTiltPosition = 0;// Current vertical tilt angle tracked in steps

// Launcher logic variables
int launcherState = 0;// The current step in the firing sequence state machine
int launcherSteps = 0;// Counter for steps during winding/unwinding phases

// Variables for managing the ball feeder without pausing the whole code
bool ballReleaseActive = false; // Tracks if the ball servo is currently in the "open" cycle
unsigned long ballReleaseStart = 0; // Records the time (ms) when the ball servo opened
const unsigned long ballReleaseDuration = 150; // How long (ms) the ball servo stays open
unsigned long startTime;// Records the time when the Arduino finished booting

void setup()// Runs once when the Arduino starts
{
  DDRA = B11111111;// Set all Port A pins (22-29) as outputs for stepper 1
  DDRC = B11111111;// Set all Port C pins (37-30) as outputs for stepper 2
  DDRB = B11111111;// Set all Port B pins (50-53/10-13) as outputs for stepper 3

  latchServo.attach(9);// Attach the latch release servo to pin 9
  latchServo.write(0);// Set initial latch position to open

  ballServo.attach(8);// Attach the ball feeder servo to pin 8
  ballServo.write(0);// Set initial ball feeder position to closed

  pinMode(triggerSwitch, INPUT_PULLUP);// Set fire button as input with internal resistor
  pinMode(leftbutton, INPUT_PULLUP);// Set left button as input with internal resistor
  pinMode(rightbutton, INPUT_PULLUP);// Set right button as input with internal resistor
  pinMode(upbutton, INPUT_PULLUP);// Set up button as input with internal resistor
  pinMode(modePin, INPUT_PULLUP); // Set mode switch as input with internal resistor

  pinMode(readyLED, OUTPUT);// Set ready indicator light as an output

  pinMode(trigFront, OUTPUT);// Set front sensor trigger pin as output
  pinMode(echoFront, INPUT);// Set front sensor echo pin as input

  pinMode(trigLeft, OUTPUT);// Set left sensor trigger pin as output
  pinMode(echoLeft, INPUT);// Set left sensor echo pin as input

  pinMode(trigRight, OUTPUT);// Set right sensor trigger pin as output
  pinMode(echoRight, INPUT);// Set right sensor echo pin as input

  attachInterrupt(digitalPinToInterrupt(leftbutton), leftISR, FALLING); //interupt for left button, triggered from high to low
  attachInterrupt(digitalPinToInterrupt(rightbutton), rightISR, FALLING);//interupt for right button, triggered from high to low
  attachInterrupt(digitalPinToInterrupt(upbutton), upISR, FALLING);//interupt for up button, triggered from high to low

  startTime = millis();// Store the current time to manage boot-up delays
  delay(100);// Wait 100ms for electronics to stabilize
  leftRequest = false;// Clear any accidental left clicks during boot
  rightRequest = false;// Clear any accidental right clicks during boot
  upRequest = false;// Clear any accidental up clicks during boot
}

long readDistance(int trigPin, int echoPin) // Function to calculate sensor distance
{
  digitalWrite(trigPin, LOW);// Ensure the trigger pin is clear
  delayMicroseconds(2);// Wait for a clean signal
  digitalWrite(trigPin, HIGH);// Send a 10-microsecond sonic pulse
  delayMicroseconds(10);// Pulse duration
  digitalWrite(trigPin, LOW);// Stop the pulse

  long duration = pulseIn(echoPin, HIGH, 40000);// Measure the duration the echo pin stays high

  if(duration == 0) return 999; // If no echo is heard, return a very large number (999cm)
  return duration * 0.034 / 2; // Calculate cm: (Time * Speed of Sound) / 2 for round-trip
}

void loop()// Runs continuously
{
  if(digitalRead(modePin) == HIGH) // If the mode jumper is not connected to ground (manual mode)
  {
    if(millis() - startTime > 200) // Ignore button presses for the first 200ms of boot
    {
      if(leftRequest)// If the left button was pressed (via interrupt)
      { 
        leftRequest = false;// Reset the flag immediately
        if(turretStepsLeft == 0 && turretStepsRight == 0)// Only move if motor is idle
        {
          turretStepsLeft = 170;// Queue 170 steps to move left
          turretPosition -= 170;// Update the net position tracker
        }
      }
      
      if(rightRequest)// If the right button was pressed (via interrupt)
      {
        rightRequest = false;// Reset the flag immediately
        if(turretStepsRight == 0 && turretStepsLeft == 0)// Only move if motor is idle
        {
          turretStepsRight = 170;// Queue 170 steps to move right
          turretPosition += 170;// Update the net position tracker
        }
      }

      if(upRequest)// If the up button was pressed (via interrupt)
      {
        upRequest = false;// Reset the flag immediately
        if(turretStepsUp == 0)// Only move if the vertical motor is idle
        {
          turretStepsUp = 370;// Queue 370 steps to tilt up
          turretTiltPosition = 370;// Update the tilt position tracker
        }
      }
    }
  }

  if(turretStepsLeft > 0)// If there are steps queued for left rotation
  { 
    rotateLeft();// Perform one physical step left
    turretStepsLeft--;// Subtract one from the remaining steps
  }

  if(turretStepsRight > 0)// If there are steps queued for right rotation
  { 
    rotateRight();// Perform one physical step right
    turretStepsRight--;// Subtract one from the remaining steps
  }

  if(turretStepsUp > 0)// If there are steps queued for tilt up
  { 
    rotateUp();// Perform one physical step up
    turretStepsUp--;// Subtract one from the remaining steps
  }

  if(turretStepsDown > 0)// If there are steps queued for tilt down (after firing)
  { 
    rotateDown();// Perform one physical step down
    turretStepsDown--;// Subtract one from the remaining steps
  }

  switch(launcherState)//launch cases
  {
    case 0: // detection case
      digitalWrite(readyLED, LOW); // Turn off the ready light while scanning
      if(digitalRead(modePin) == HIGH) // If in Manual mode
      {
        launcherState = 1;// Jump straight to winding the ruler
      }
      else// If in Autonomous mode ( pin = 0 )
      {
        distanceFront = readDistance(trigFront, echoFront); // Scan front
        distanceLeft  = readDistance(trigLeft, echoLeft);// Scan left
        distanceRight = readDistance(trigRight, echoRight); // Scan right
        int detectRange = 300;// Target must be within 300m to fire

        if(distanceLeft < detectRange && distanceLeft < distanceRight) // Target on left
        {
          turretStepsLeft += 80;// Turn 80 steps toward target (or 640 half-steps)
          turretPosition -= 80;// Track position
          launcherState = 1;// Start winding
        }
        else if(distanceRight < detectRange && distanceRight < distanceLeft) // Target on right
        {
          turretStepsRight += 80;// Turn 80 steps toward target (or 640 half-steps)
          turretPosition += 80;// Track position
          launcherState = 1;// Start winding
        }
        else if(distanceFront < detectRange)// Target directly in front
        {
          launcherState = 1;// Start winding
        }
      }
      break;//breaking code

    case 1: // WINDING PHASE
      clockwise1();                // Drive motor A forward to pull ruler back
      launcherSteps++;             // Increment steps taken
      if(launcherSteps >= 530)     // If ruler is fully pulled back
      { 
        launcherSteps = 0;         // Reset step counter
        launcherState = 2;         // Move to loading ball
      }
      break;//breaking code

    case 2: //ball loading 
      latchServo.write(90);// Open the latch so it can grab the ruler
      if(!ballReleaseActive)// If ball servo isn't already busy
      {
        ballServo.write(90);// Move ball servo to release one ball
        ballReleaseActive = true;// Mark servo as busy
        ballReleaseStart = millis(); // Record time for the timer
        delay(300);// Wait for ball to physically drop
      }
      launcherState = 3;// Move to releasing tension
      break;//breaking code

    case 3://tension release
      counterclockwise1();// Drive motor A backward to transfer tension to latch
      launcherSteps++;// Increment steps
      if(launcherSteps >= 530)// If motor has returned to neutral
      {
        launcherSteps = 0;// Reset step counter
        ballServo.write(0);// Ensure ball gate is closed
        ballReleaseActive = false; // Mark servo as free
        launcherState = 4; // Move to waiting for trigger
        digitalWrite(readyLED, HIGH); // Light up LED
      }
      break;//breaking code

    case 4: //waiting for buttons
      if(digitalRead(modePin) == HIGH) // If Manual Mode
      {
        if(digitalRead(triggerSwitch) == LOW) { launcherState = 5; } // Wait for button press
      }
      else { launcherState = 5; }  // If Auto Mode, fire immediately
      break;//breaking code

    case 5: //fire ball and reset to "home state"
      latchServo.write(0);// Snap the latch back to 0 (releasing the ruler)

      if(turretPosition > 0)  turretStepsLeft += turretPosition; // If right, move left
      else if(turretPosition < 0) turretStepsRight += -turretPosition; // If left, move right
      turretPosition = 0;// Horizontal center is restored

      if(digitalRead(modePin) == HIGH && turretTiltPosition > 0)//if up, move down
      { 
        turretStepsDown += turretTiltPosition; // Queue steps to tilt back down
        turretTiltPosition = 0;// Vertical center is restored
      }

      while(turretStepsLeft>0 || turretStepsRight>0 || turretStepsUp>0 || turretStepsDown>0)//making motors finish before continuing 
      {
        if(turretStepsLeft>0)//if there are steps in counter  
        { 
        rotateLeft();//rotate back to center
        turretStepsLeft--; //decrease to reach 0
        }

        if(turretStepsRight>0) //if there are steps in counter  
        { 
        rotateRight();//rotate back to center
        turretStepsRight--;//decrease to reach 0
        }

        if(turretStepsUp>0)//if there are steps in counter      
        { 
        rotateUp();//rotate back to center
        turretStepsUp--;//decrease to reach 0
        }

        if(turretStepsDown>0)//if there are steps in counter   
        { 
        rotateDown();//rotate back to center
        turretStepsDown--;//decrease to reach 0
        }
      }

      if(digitalRead(modePin) == LOW)//if in auto mode
      { 
      delay(20000); // 20s safety cooldown in auto mode
      } 

      launcherState = 0; // go back to the beginning
      break;//breaking case
  }

  if(ballReleaseActive && millis() - ballReleaseStart >= ballReleaseDuration)// If the ball servo is open, check if the duration (150ms) has passed
  {
    ballServo.write(0);// Close the ball gate
    ballReleaseActive = false;// Reset the flag
  }
}

void upISR()//interupt for up 
{ 
upRequest = true; // Triggered by Up button
}  

void leftISR()//interupt for left turning 
{ 
leftRequest = true;// Triggered by Left button
} 

void rightISR()//interupt for right turning 
{ 
rightRequest = true;// Triggered by Right button
}

void counterclockwise1()// Stepper Motor A CCW (Loosen)
{
  PORTA=B00001001; delay(tf);// Step 1
  PORTA=B00001000; delay(tf);// Step 2
  PORTA=B00001100; delay(tf);// Step 3
  PORTA=B00000100; delay(tf);// Step 4
  PORTA=B00000110; delay(tf);// Step 5
  PORTA=B00000010; delay(tf);// Step 6
  PORTA=B00000011; delay(tf);// Step 7
  PORTA=B00000001; delay(tf);// Step 8
}

void clockwise1()// stepper Motor A CW (Wind up)
{
  PORTA=B00000001; delay(t);// Step 1
  PORTA=B00000011; delay(t);// Step 2
  PORTA=B00000010; delay(t);// Step 3
  PORTA=B00000110; delay(t);// Step 4
  PORTA=B00000100; delay(t);// Step 5
  PORTA=B00001100; delay(t);// Step 6
  PORTA=B00001000; delay(t);// Step 7
  PORTA=B00001001; delay(t);// Step 8
}

void rotateRight()// Stepper Motor C (Rotation Right)
{
  PORTC=B00001001; delay(tl);// Step 1
  PORTC=B00001000; delay(tl);// Step 2
  PORTC=B00001100; delay(tl);// Step 3
  PORTC=B00000100; delay(tl);// Step 4
  PORTC=B00000110; delay(tl);// Step 5
  PORTC=B00000010; delay(tl);// Step 6
  PORTC=B00000011; delay(tl);// Step 7
  PORTC=B00000001; delay(tl);// Step 8
}

void rotateLeft()// Stepper Motor C (Rotation Left)
{
  PORTC=B00000001; delay(tl);// Step 1
  PORTC=B00000011; delay(tl);// Step 2
  PORTC=B00000010; delay(tl);// Step 3
  PORTC=B00000110; delay(tl);// Step 4
  PORTC=B00000100; delay(tl);// Step 5
  PORTC=B00001100; delay(tl);// Step 6
  PORTC=B00001000; delay(tl);// Step 7
  PORTC=B00001001; delay(tl);// Step 8
}

void rotateUp()// Stepper Motor B (Tilt Up)
{
  PORTB=B00001000; delay(tUD);// Step 1
  PORTB=B00001100; delay(tUD);// Step 2
  PORTB=B00000100; delay(tUD);// Step 3
  PORTB=B00000110; delay(tUD);// Step 4
  PORTB=B00000010; delay(tUD);// Step 5
  PORTB=B00000011; delay(tUD);// Step 6
  PORTB=B00000001; delay(tUD);// Step 7
  PORTB=B00001001; delay(tUD);// Step 8
}

void rotateDown()// Stepper Motor B (Tilt Down)
{
  PORTB=B00001001; delay(tUD);// Step 1
  PORTB=B00000001; delay(tUD);// Step 2
  PORTB=B00000011; delay(tUD);// Step 3
  PORTB=B00000010; delay(tUD);// Step 4
  PORTB=B00000110; delay(tUD);// Step 5
  PORTB=B00000100; delay(tUD);// Step 6
  PORTB=B00001100; delay(tUD);// Step 7
  PORTB=B00001000; delay(tUD);// Step 8
}
