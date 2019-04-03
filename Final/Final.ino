#include <Servo.h>  

#define lineSensor2 49  // far left
#define lineSensor3 51  // near left
#define lineSensor4 53  // near right
#define lineSensor5 52  // far right

/**
 * Port# define
 */
const int pingPin = 2;
const int blue = 44;
const int red = 45;
const int green = 46;

int pb = 7;
int tx = 3;
int rx = 4;

Servo servoLeft;
Servo servoRight;

int black_time = 0;							   // # of crosses has passed
int sampling_t = 10;                           // period

int threshold = 100;                           // < threshold (0) for white, > threshold (1) for black
int binary[4] = {0, 0, 0, 0};                  // binary numbers for each sensor read
int code = 1111;                               // integrated binary read

int precode = code;                            // TEST: last different binary read
int was_black  = 0;                            // was the last check black

bool DEBUG = false;

void setup() { 
  // initialize LEDs                
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);                      

  digitalWrite(red, HIGH);
  digitalWrite(blue, HIGH);
  digitalWrite(green, HIGH);
  
  // XBee setup
  pinMode(pb, INPUT);                        // Push button input
  pinMode(tx, OUTPUT);                       // Transmit LED
  pinMode(rx, OUTPUT);                       // Receive LED

  Serial.begin(9600);
  Serial2.begin(9600);

  // Servos setup 
  attach_motors();
  delay(500);
}

void loop() {
  if (black_time == 6) { 					// At the end of the line 
    servoLeft.detach();  
    servoRight.detach();    		  // Detach servos after reaching the last cross 

    while(true) {
      did_receive();						  // Wait to receive a signal from anthoer bot
    }
  }
  
  /* Normal loop starts here */
  convert_binary();                         // convert readings into a binary code 
  code = binary[0] * 1000 + binary[1] * 100 + binary[2] * 10 + binary[3]; // convert to an integer
  	
  // Print out debug output if necessary
  if (DEBUG) {
  	debug_output();
  }

  robot_move(code);
  delay(sampling_t);
}

/* Helper methods start here */

/**
 * Read in sensor values and convert to a binary array
 */
void convert_binary() {
  int rawVals[4];
  
  rawVals[0] = RCTime(lineSensor2);    
  rawVals[1] = RCTime(lineSensor3);     
  rawVals[2] = RCTime(lineSensor4);     
  rawVals[3] = RCTime(lineSensor5);     

  // convert time to binary numbers
  for (int i = 0; i < 4; i++) {
    if (rawVals[i] > threshold) 
      binary[i] = 1;
    else
      binary[i] = 0;
  }
  
}

/**
 * Robot move under differnet conditions
 * Note: 1500, 1500 stops the robot 
 *       1600, 1600 robot moves forward full speed
 */
void robot_move(int code){
   switch(code) {
    case 1000:  // pivot left 
      simple_move(1450, 1600);
      break;
    case 1100:  // moving left 
      simple_move(1500, 1600);
      break;
    case 1110:  // left corner
      simple_move(1400, 1650);
      break;
    case 100:   // slight left 
      simple_move(1600, 1600);
      break;
    case 110:   // forward 
      simple_move(1600, 1600);
      break; 
    case 10:    // slight right 
      simple_move(1600, 1600);
      break;
    case 111:   // right corner 
      simple_move(1650, 1400);
      break;
    case 11:    // moving right 
      simple_move(1600, 1500);
      break;
    case 1:     // right pivot 
      simple_move(1600, 1450);
      break;
    case 1111:  // stop
      if (was_black) {									// if reads a cross the last time, move forward before reading the next value
        simple_move(1600, 1600);
        delay(300);
        was_black = 0;
      } else {													// if reads a cross
        simple_move(1500, 1500);				// stop
        delay(1000);
        detach_motors();
        detectQuaffle(); 								// detect whether a Quaffle is present
        delay(1000);
        attach_motors();
        delay(1000);
        simple_move(1600, 1600);				// keep moving
        was_black = 1;
        black_time ++;									// record the # of crosses
      }
      break;
    case 0:		// the end of a line (for line tracking demo only)														
        simple_move(1450, 1450);
        delay(100);
        Serial.println("All 0s");
        break;
    default:	// abnormal input
        Serial.println("Warning! Abnormal input, check qtis");
        break;
  }
}

/**
 * Detect whether a quaffle is present with an ultrasonic sensor
 */
void detectQuaffle() {
  long duration, inches;
  
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);

  // print out the distance detected
  if (DEBUG) {
  	Serial.print(inches);
  	Serial.print("in, ");
  }
  
  // if Quaffle detected, send a signal and LED blinks
  if (inches < 10) { 
    send_character('R'); 
    send_character('K'); 
    delay(500);              
    lightup();
  } else {
    send_character('N');
  }
}

/* Secondary helper methods start here */
/**
 * Convert ultrasonic sensor input to distance
 */
long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}

/**
 * Convert PWM input into humane input value
 */
void simple_move(int left, int right) {
  servoLeft.writeMicroseconds(left);        
  servoRight.writeMicroseconds(3000 - right);
}

/**
 * LED labor
 * Note: for this LED, HIGH - OFF
 *										 LOW  - ON
 */
void lightup(){
  for (int i = 0; i < 3; i++){
    digitalWrite(red, LOW);					// red	
    delay(100); 
    digitalWrite(green, LOW);				// yellow
    delay(100); 
    digitalWrite(red, HIGH);				// green
    delay(100); 
    digitalWrite(green, HIGH);			// off
    digitalWrite(blue, LOW);				// blue
    delay(100); 
    digitalWrite(red, LOW);					// purple
    delay(100);
    digitalWrite(red, HIGH);
    digitalWrite(blue, HIGH);
    digitalWrite(green, HIGH);			// no pink i am sad DX
  }
}

/**
 * Process the sensor discharging time
 */
long RCTime(int sensorIn){
  long duration = 0;
  pinMode(sensorIn, OUTPUT);     // Sets pin as OUTPUT
  digitalWrite(sensorIn, HIGH);  // Pin HIGH
  delay(1);                      // Waits for 1 millisecond
  pinMode(sensorIn, INPUT);      // Sets pin as INPUT
  digitalWrite(sensorIn, LOW);   // Pin LOW
 
  while(digitalRead(sensorIn)) { // Waits for the pin to go LOW
    duration++;
  }
  
  return duration;               // Returns the duration of the pulse
}

/**
 * Send a 'R' to the sensory bot
 */
void send_character(char x) {
    char outgoing = x; 
    Serial2.print(outgoing);  // Send a character 
    digitalWrite(tx, HIGH);   // LED lights up for transimission
    Serial.println(outgoing); // Also indicate in the local Serial window
    delay(500);
    digitalWrite(tx, LOW);
}

/**
 * Light up a LED and print if receives anything
 */
void did_receive() {
    if(Serial2.available()) {   			// If a character is received
    char ingoing = Serial2.read();
    digitalWrite(rx, HIGH); 					// LED lights up for receiving
    Serial.println(ingoing);					// Print on the serial monitor
    delay(500);
    digitalWrite(rx, LOW);						// LED off
    
  }
}

/**
 * Print code in debug mode
 */
void debug_output() {
	if (code != precode) {
	    Serial.println(code);
	    precode = code;
	    Serial.print("qti code: ");
	    Serial.println(precode);
  	}
}

void detach_motors() {
  servoLeft.detach();
  servoRight.detach();
}

void attach_motors() {
  servoLeft.attach(11);                      // Attach left signal to pin 11
  servoRight.attach(12);                     // Attach right signal to pin 12
}
