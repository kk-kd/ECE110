#include <Servo.h>  

Servo servoLeft;
Servo servoRight;

int r = 0;

void setup() {
  Serial.begin(9600);
  servoLeft.attach(11);                      // Attach left signal to pin 11
  servoRight.attach(12);

}

void loop() {
  simple_move(1500, 1500);
  delay (100);
}

void simple_move(int left, int right) {
  servoLeft.writeMicroseconds(left);        
  servoRight.writeMicroseconds(3000 - right);
}
