#define Rx 17
#define Tx 16

int pb = 3;
int green = 5;
int red = 4;

void setup() {
  pinMode(pb, INPUT);  // Push button input
  pinMode(green, OUTPUT); // Tx Green LED
  pinMode(red, OUTPUT); // Rx Red LED
  
  Serial.begin(9600);
  Serial2.begin(9600);

  digitalWrite(green, HIGH); // Successful setup
  delay(500);
}

void loop() {
  if (digitalRead(pb) == 0) { // If the pushbutton is pressed
    Serial.println('a'); // Send the character a
    Serial2.println('b');
    digitalWrite(green, HIGH); // Green LED light up for Tx  
  }

  if (Serial2.available()) { //If a character is received
    char received = Serial2.read();
    digitalWrite(red, HIGH); // Red LED light up for Rx
    Serial.println(received); 
  }
  
  delay(50);  
  
  digitalWrite(green, LOW);
  digitalWrite(red, LOW); //turn both LEDs off
}
