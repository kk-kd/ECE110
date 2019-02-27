int pb = 7;
int tx = 2;
int rx = 5;

void setup() {
  pinMode(pb, INPUT);  // Push button input
  pinMode(tx, OUTPUT); // Transmit LED
  pinMode(rx, OUTPUT); // Receive LED

  Serial.begin(9600);
  Serial2.begin(9600);
  delay(500);
}

void loop() {
  if(digitalRead(pb)) {       // If the pushbutton is pressed
    char outgoing = 'P'; 
    Serial2.print(outgoing);  // Send a character 'P'
    digitalWrite(tx, HIGH);   // LED lights up for transimission
    Serial.println(outgoing); // Also indicate in the local Serial window
    delay(500);
  }
  
  if(Serial2.available()) {   // If a character is received
    char ingoing = Serial2.read();
    digitalWrite(rx, HIGH); // LED lights up for receiving
    Serial.println(ingoing);
    delay(500);
  }
  
  delay(50);  // Delay for short time
  digitalWrite(tx, LOW); 
  digitalWrite(rx, LOW);      // Turn both LEDs off
}
