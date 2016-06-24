/* --- Libraries --- */

/* --- Serial / Commands --- */
const int BAUD = 9600;

/* --- I/O Pins --- */
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  int center_line = analogRead(CENTER_LINE_PIN);
  int left_line = analogRead(LEFT_LINE_PIN);
  int right_line = analogRead(RIGHT_LINE_PIN);
  Serial.print(left_line); 
  Serial.print(' ');
  Serial.print(center_line); 
  Serial.print(' ');
  Serial.print(right_line); 
  Serial.println(' ');
}

