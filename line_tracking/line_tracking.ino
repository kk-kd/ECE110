#include <Servo.h>  

#define lineSensor2 49  // far left
#define lineSensor3 51  // near left
#define lineSensor4 53  // near right
#define lineSensor5 52  // far right

Servo servoLeft;
Servo servoRight;

int threshold = 80;                           // < threshold (0) for white, > threshold (1) for black
int binary[4] = {0, 0, 0, 0};                 // binary numbers for each sensor read
int code = 1111;                              // integrated binary read

void setup() {
  tone(4, 3000, 1000);                       // Play tone for 1 second
  delay(1000);                               // Delay to finish tone

  Serial.begin(9600);         
  servoLeft.attach(11);                      // Attach left signal to pin 11
  servoRight.attach(12);                     // Attach right signal to pin 12
}

void loop() {
  convert_binary();   
  
  Serial.println(binary[0]); 
  Serial.println(binary[1]); 
  Serial.println(binary[2]); 
  Serial.println(binary[3]);

  code = binary[0] * 1000 + binary[1] * 100 + binary[2] * 10 + binary[3];
  Serial.println(code);

  // test
  simple_move(1500, 1500);

  // robot_move(code);
  delay(50);

  
}

/**
 * Robot move under differnet conditions
 * Note: 1500, 1500 stops the robot 
 *       1700, 1300 robot moves forward
 */
void robot_move(code){
   switch(code) {
    case 1000:  // pivot left 
      simple_move(1500, 1300);
      break;
    case 1100:  // moving left 
      simple_move(1600, 1300);
      break;
    case 1110:  // left corner
      simple_move(1300, 1300);
      break;
    case 0100:  // slight left 
      simple_move(1700, 1300);
      break;
    case 1100:  // forward 
      simple_move(1700, 1300);
      break; 
    case 0010:  // slight right 
      simple_move(1700, 1300);
      break;
    case 0111:  // right corner 
      simple_move(1700, 1700);
      break;
    case 0011:  // moving right 
      simple_move(1700, 1400);
      break;
    case 0001:  // right pivot 
      simple_move(1700, 1500);
      break;
    case 1111:  // stop
       simple_move(1500, 1500);
       break;
    default:
        Serial.println("Warning. Check qtis");
        break;
  }
}

void simple_move(int left, int right) {
  servoLeft.writeMicroseconds(left);        
  servoRight.writeMicroseconds(right);
}

/**
 * Read in sensor values and convert to a binary array
 */
void convert_binary() {
  int rawVals[4];
  
  rawVals[0] = RCTime(lineSensor2);     //Calls funtion 'RCTime' Request reading from QTI sensor at pin 'linesensor1' saves value in variable 'qti'
  rawVals[1] = RCTime(lineSensor3);     //Calls funtion 'RCTime' Request reading from QTI sensor at pin 'linesensor1' saves value in variable 'qti'
  rawVals[2] = RCTime(lineSensor4);     //Calls funtion 'RCTime' Request reading from QTI sensor at pin 'linesensor1' saves value in variable 'qti'
  rawVals[3] = RCTime(lineSensor5);     //Calls funtion 'RCTime' Request reading from QTI sensor at pin 'linesensor1' saves value in variable 'qti'
  
  Serial.println(rawVals[0]);  //output for testing purpose
  Serial.println(rawVals[1]); 
  Serial.println(rawVals[2]); 
  Serial.println(rawVals[3]);

  // convert time to binary numbers
  for (int i = 0; i < 4; i++) {
    if (rawVals[i] > threshold) 
      binary[i] = 0;
    else
      binary[i] = 1;
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
