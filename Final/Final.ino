#include <Servo.h>  
#include <SoftwareSerial.h>


/**
 * Port# define
 */
#define lineSensor2 49  // far left
#define lineSensor3 51  // near left
#define lineSensor4 53  // near right
#define lineSensor5 52  // far right
#define LCDTx 14        // LCD

#define pingPin 2
#define blue 44
#define red 45
#define green 46

#define tx 3
#define rx 4

/**
 * Components define
 */
SoftwareSerial serialLCD = SoftwareSerial(255, LCDTx);  // Setup LCD screen
Servo servoLeft;                                        
Servo servoRight;

/**
 * Variables define
 */
int black_time = 0;                            // # of crosses has passed
int sampling_t = 10;                           // Period

int threshold = 100;                           // < threshold (0) for white, > threshold (1) for black
int binary[4] = {0, 0, 0, 0};                  // Binary numbers for each sensor read
int code = 1111;                               // integrated binary read

int precode = code;                            // TEST: last different binary read
int was_black  = 0;                            // was the last check black?

bool DEBUG = true;                            // Whether turn on debug output

int fs[5] = {0, 0, 0, 0, 0};                   // Array for recording Quaffle detection status, 1 if present
int group_score_1 = 0;                         // Score for Beater Chaser 1
int group_score_2 = 0;                         // Score for Beater Chaser 2   
int seeker_score = 0;                          // Score for Seeker
int final_score = 0;

void setup() { 
  // initialize LEDs                
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);                      

  digitalWrite(red, HIGH);
  digitalWrite(blue, HIGH);
  digitalWrite(green, HIGH);
  
  // XBee setup
  pinMode(tx, OUTPUT);                       // Transmit LED
  pinMode(rx, OUTPUT);                       // Receive LED

  Serial.begin(9600);
  Serial2.begin(9600);
  
  // LCD Setup
  pinMode(LCDTx, OUTPUT);
  digitalWrite(LCDTx, HIGH);
  serialLCD.begin(9600);
  serialLCD.write(12);
  serialLCD.write(18);
  delay(5);
  serialLCD.print("Hello Driver!");
  
  // Happy time
  playsound();

  // Servos setup 
  attach_servos();
  delay(500);
}

void loop() 
{
  if (black_time == 5) {          // At the end of the line 
    simple_move(1450, 1450);      // move backwards, giving space to the seeker
    delay(2000);
    simple_move(1500,1500);
    detach_servos();              // Detach servos after reaching the last cross

    transmitScores(fs);                         // transmit scores
    int** team_scores = receiveScores(fs);      // and then try to receive scores
    
    // calculate scores for each team and add them together
    calc_score_1(team_scores); 
    calc_score_2(team_scores);
    calc_score_3(team_scores);
    total_score_calc();

// TODO: figure this out
//    String send_final_score = "g" + final_score;
//    Serial2.print(send_final_score);

    if (DEBUG)
    {
      print_team_scores(team_scores);
    }
   
    // SerialLCD displays score
    serialLCD.write(12);
    serialLCD.print("Final Score:");
    serialLCD.print(final_score);
    serialLCD.write(13);
    serialLCD.print(group_score_1);
    serialLCD.print(" ");
    serialLCD.print(group_score_2);
    serialLCD.print(" ");
    serialLCD.print(seeker_score);

    while(true){}    // stop
  }
  
  /* Normal loop starts here */
  convert_binary();                                                       // convert readings into a binary code 
  code = binary[0] * 1000 + binary[1] * 100 + binary[2] * 10 + binary[3]; // and then convert to an integer
    
  // Print out debug output if necessary
  if (DEBUG) 
  {
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
      if (was_black) {                  // if reads a hash mark the last time, move forward before reading the next value
        simple_move(1600, 1600);
        delay(300);
        was_black = 0;
      } else {                          // if reads a hash mark
        simple_move(1500, 1500);        // stop
        delay(1000);
        detach_servos();
        detectQuaffle();                // detect whether a Quaffle is present
        delay(1000);
        attach_servos();
        delay(1000);
        simple_move(1600, 1600);        // keep moving
        was_black = 1;
        black_time ++;                  // record the # of hash marks
      }
      break;
    case 0:   // the end of a line (for line tracking demo only)                            
        simple_move(1450, 1450);
        delay(100);
        Serial.println("All 0s");
        break;
    default:  // abnormal input
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
  if (DEBUG) 
  {
    Serial.print(inches);
    Serial.print("in, ");
  }
  
  // store the quaffle info in an array
  if (inches < 10) 
  { 
    fs[black_time] = 1;
    delay(500);              
    lightup();
  } 
  else 
  {
    fs[black_time] = 0;
  }
}

/* Secondary helper methods start here */
/**
 * Convert ultrasonic sensor input to distance
 */
long microsecondsToInches(long microseconds) 
{
  return microseconds / 74 / 2;
}

/**
 * Convert PWM input into humane input value
 */
void simple_move(int left, int right) 
{
  servoLeft.writeMicroseconds(left);        
  servoRight.writeMicroseconds(3000 - right);
}

/**
 * LED labor
 * Note: for this LED, HIGH - OFF
 *                     LOW  - ON
 */
void lightup()
{
  for (int i = 0; i < 3; i++){
    digitalWrite(red, LOW);         // red  
    delay(100); 
    digitalWrite(green, LOW);       // yellow
    delay(100); 
    digitalWrite(red, HIGH);        // green
    delay(100); 
    digitalWrite(green, HIGH);      // off
    digitalWrite(blue, LOW);        // blue
    delay(100); 
    digitalWrite(red, LOW);         // purple
    delay(100);
    digitalWrite(red, HIGH);
    digitalWrite(blue, HIGH);
    digitalWrite(green, HIGH);      // no pink i am sad DX
  }
}

/**
 * Process the sensor discharging time
 */
long RCTime(int sensorIn)
{
  long duration = 0;
  pinMode(sensorIn, OUTPUT);     // Sets pin as OUTPUT
  digitalWrite(sensorIn, HIGH);  // Pin HIGH
  delay(1);                      // Waits for 1 millisecond
  pinMode(sensorIn, INPUT);      // Sets pin as INPUT
  digitalWrite(sensorIn, LOW);   // Pin LOW
 
  while(digitalRead(sensorIn)) 
  { // Waits for the pin to go LOW
    duration++;
  }
  
  return duration;               // Returns the duration of the pulse
}

/**
 * Print qti code in debug mode
 */
void debug_output() 
{
  if (code != precode) 
  {
      Serial.println(code);
      precode = code;
      Serial.print("qti code: ");
      Serial.println(precode);
    }
}

void detach_servos() 
{
  servoLeft.detach();
  servoRight.detach();
}

void attach_servos()
{
  servoLeft.attach(11);                      // Attach left signal to pin 11
  servoRight.attach(12);                     // Attach right signal to pin 12
}

/**
 * Harry Potter!
 */
void playsound() 
{
  serialLCD.write(216); //A = 440
  serialLCD.write(212); //1/4 note
  serialLCD.write(222); //B
  serialLCD.write(227); //E
  serialLCD.write(211); //1/8 note
  serialLCD.write(230); //G
  serialLCD.write(212); //1/4 note
  serialLCD.write(229); //F#
  serialLCD.write(213); //1/2 note
  serialLCD.write(227); //E
  serialLCD.write(212); //1/4 note
  serialLCD.write(217); //A=880
  serialLCD.write(222); //B
  serialLCD.write(213); //1/2 note
  serialLCD.write(220); //A
  serialLCD.write(216); //A=440
  serialLCD.write(229); //F#
}


/**
 * Transmit our Quaffle detection in an array
 */
void transmitScores(int score[]) 
{
   String message = "c";                   // identifier, c for the third line

   for (int i = 0; i < 5; i++) {
    message += score[i];                   // prepare message for sending
   }
   Serial.print(message);
   
   for (int i = 0; i < 1; ++i)
   {
     Serial2.print(message);               // send the message various times to be cautious
   }
}
 
/**
 * Receive scores from the rest of the team 
 */
int** receiveScores(int* your_score) {
  int count = 1;
  long start = millis();
  int team_index = 2;                     
  
  int received[5] = {0, 0, 1, 0, 0};            // 0 for not received yet, 1 for received 
 
  // Array containing the scores for the team
  int** team_scores = { 0 };
  team_scores = new int*[5];

  // Fill in the array
  for (int i = 0; i < 5; i++) 
  {
     team_scores[i] = new int[5];
      if (i != team_index) {
          for (int j = 0; j < 5; j++) 
          {
              team_scores[i][j] = 0;
          }
      } 
      else 
      {
          for (int j = 0; j < 5; j++) 
          {
              team_scores[i][j] = your_score[j];
          }
      }
  }
 
  // While all scores have yet to be received
  // Or forty seconds have yet to pass
  while(count < 5 && millis() - start < 40000) 
  {
    // If there is at least a message worth of data available
    if (Serial2.available() >= 6) 
    { 
      // Calculate the team index of the message
      int index = Serial2.read() - 97;
      // If we read an invalid team # try again
      if (index < 0 || index >= 5) 
      {
        continue;
      }
      // If we haven't received this teams score yet
      else if (received[index] == 0) 
      {
        // Mark as received, print out for testing
        received[index] = 1;
        Serial.print(index);

        // Fill in the matrix
        for (int i = 0; i < 5; i++) 
        {
          team_scores[index][i] = Serial2.read() - 48;
           Serial.print(team_scores[index][i]);
        }
        Serial.println();
        count++;
        
        // Print out number of received scores
        Serial.print("Count: ");
        Serial.println(count);
      }
    }
  }

  Serial.println("out of loop");
  return team_scores;
}

/**
 * Calculate score of team 1
 */
void calc_score_1(int** team_score) 
{
  for (int i = 0; i < 5; i++)
  {
    if (team_score[0][i] == 1 && team_score[1][i] == 1)
    {
      group_score_1 += 10;
    }
  }
}

/**
 * Calculate score of team 2
 */
void calc_score_2(int** team_score) 
{
  for (int i = 0; i < 5; i++)
  {
    if (team_score[2][i] == 1 && team_score[3][i] == 1)
    {
      group_score_2 += 10;
    }
  }
}

/**
 * Calculate score of team 3
 */
void calc_score_3(int** team_score) {
  for (int i = 0; i < 5; i++)
  {
    if (team_score[4][i] == 1)
    {
      seeker_score += 150;
      break;
    }
  }
}

/**
 * Calculate total score 
 */
void total_score_calc() 
{
  final_score = group_score_1 + group_score_2 + seeker_score;
}


/**
 * Print out team scores in the serial monitor
 */
void print_team_scores(int **team_scores) 
{
  for (int i = 0; i < 5; i++)
    {
      for (int j = 0; j < 5; j++)
      {
        Serial.print(team_scores[i][j]);
      }
      Serial.println();
    }

    Serial.print("final_score: ");
    Serial.println(final_score);
}
