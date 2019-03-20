#include <Servo.h>  

#define lineSensor2 49  // far left
#define lineSensor3 51  // near left
#define lineSensor4 53  // near right
#define lineSensor5 52  // far right

Servo servoLeft;
Servo servoRight;

int black_time = 0;
int threshold = 100;                           // < threshold (0) for white, > threshold (1) for black
int binary[4] = {0, 0, 0, 0};                  // binary numbers for each sensor read
int code = 1111;                               // integrated binary read
int precode = code;                            // TEST: last different binary read
int was_black  = 0;                            // was the last check black
int sampling_t = 10;                           // period

const int pingPin = 2;
const int blue = 44;
const int red = 45;
const int green = 46;

int pb = 7;
int tx = 3;
int rx = 4;

void setup() {                 
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);                      // initialize a LED

  digitalWrite(red, HIGH);
  digitalWrite(blue, HIGH);
  digitalWrite(green, HIGH);
  
  pinMode(pb, INPUT);                        // Push button input
  pinMode(tx, OUTPUT);                       // Transmit LED
  pinMode(rx, OUTPUT);                       // Receive LED

  Serial.begin(9600);
  Serial2.begin(9600);
 
  servoLeft.attach(11);                      // Attach left signal to pin 11
  servoRight.attach(12);                     // Attach right signal to pin 12
   
  delay(500);
}

void loop() {
  if (black_time == 6) {
    servoLeft.detach();  
    servoRight.detach();    //detach servos after reaching the end 

    while(true) {
      did_receive();
    }
  }
  
  convert_binary();                         // convert readings into a binary code 

//  TEST
//  Serial.println(binary[0]); 
//  Serial.println(binary[1]); 
//  Serial.println(binary[2]); 
//  Serial.println(binary[3]);

  code = binary[0] * 1000 + binary[1] * 100 + binary[2] * 10 + binary[3]; // convert to an integer

  // TEST: update 
  if (code != precode) {
    Serial.println(code);
    precode = code;
  }

  robot_move(code);
  delay(sampling_t);
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
    case 100:  // slight left 
      simple_move(1600, 1600);
      break;
    case 110:  // forward 
      simple_move(1600, 1600);
      break; 
    case 10:  // slight right 
      simple_move(1600, 1600);
      break;
    case 111:  // right corner 
      simple_move(1650, 1400);
      break;
    case 11:  // moving right 
      simple_move(1600, 1500);
      break;
    case 1:  // right pivot 
      simple_move(1600, 1450);
      break;
    case 1111:  // stop
      if (was_black) {
        simple_move(1600, 1600);
        delay(300);
        was_black = 0;
      } else {
        simple_move(1500, 1500);
        detectQuaffle();
        delay(2000);
        simple_move(1600, 1600);
        was_black = 1;
        black_time ++;
      }
      break;
    case 0:
        simple_move(1450, 1450);
        delay(100);
        Serial.println("All 0s");
        break;
    default:
        Serial.println("Warning! Abnormal input, check qtis");
        break;
  }
}

void send_character() {
    char outgoing = 'l'; 
    Serial2.print(outgoing);  // Send a character 'P'
    digitalWrite(tx, HIGH);   // LED lights up for transimission
    Serial.println(outgoing); // Also indicate in the local Serial window
    delay(500);
    digitalWrite(tx, LOW);
}

void did_receive() {
    if(Serial2.available()) {   // If a character is received
    char ingoing = Serial2.read();
    digitalWrite(rx, HIGH); // LED lights up for receiving
    Serial.println(ingoing);
    delay(100);
    digitalWrite(rx, LOW);
    
  }
}

void simple_move(int left, int right) {
  servoLeft.writeMicroseconds(left);        
  servoRight.writeMicroseconds(3000 - right);
}

/**
 * Read in sensor values and convert to a binary array
 */
void convert_binary() {
  int rawVals[4];
  
  rawVals[0] = RCTime(lineSensor2);    
  rawVals[1] = RCTime(lineSensor3);     
  rawVals[2] = RCTime(lineSensor4);     
  rawVals[3] = RCTime(lineSensor5);     
  
//  Serial.println(rawVals[0]);  //output for testing purpose
//  Serial.println(rawVals[1]); 
//  Serial.println(rawVals[2]); 
//  Serial.println(rawVals[3]);

  // convert time to binary numbers
  for (int i = 0; i < 4; i++) {
    if (rawVals[i] > threshold) 
      binary[i] = 1;
    else
      binary[i] = 0;
  }
  
}

void detectQuaffle() {
  long duration, inches, cm;
  
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
  cm = microsecondsToCentimeters(duration);

  // print out the distance detected
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();

  // if Quaffle detected, LED blinks
  if (inches < 10) { 
    send_character();                 
    lightup();
  } 
}

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

void lightup(){
  for (int i = 0; i < 5; i++){
    digitalWrite(red, LOW);
    delay(100); 
    digitalWrite(green, LOW);
    delay(100); 
    digitalWrite(red, HIGH);
    delay(100); 
    digitalWrite(green, HIGH);
    digitalWrite(blue, LOW);
    delay(100); 
    digitalWrite(red, LOW);
    delay(100);
    digitalWrite(red, HIGH);
    digitalWrite(blue, HIGH);
    digitalWrite(green, HIGH);
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
