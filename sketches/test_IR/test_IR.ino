/* --- Libraries --- */

/* --- Serial / Commands --- */
const int BAUD = 9600;

/* --- I/O Pins --- */
const int IR_DIST_PIN = A3;

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(IR_DIST_PIN, INPUT);
}

/* --- Loop --- */
void loop() {
  int dist = analogRead(IR_DIST_PIN);
  delay(100);
  Serial.println(dist);
}

