/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/* --- Constants --- */
const int LINE_THRESHOLD = 750;
const int BAUD = 9600;
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A1;
const int CENTER_LINE_PIN = A2;

const int FRONT_LEFT_SERVO = 0;
const int FRONT_RIGHT_SERVO = 1;
const int BACK_LEFT_SERVO = 2;
const int BACK_RIGHT_SERVO = 3;

const int FRONT_LEFT_MIN = 300;
const int FRONT_LEFT_OFF = 381; // this is the servo off pulse length
const int FRONT_LEFT_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)

const int FRONT_RIGHT_MIN = 300;
const int FRONT_RIGHT_OFF = 381; // this is the servo off pulse length
const int FRONT_RIGHT_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)

const int BACK_LEFT_MIN = 300;
const int BACK_LEFT_OFF = 381; // this is the servo off pulse length
const int BACK_LEFT_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)

const int BACK_RIGHT_MIN = 300;
const int BACK_RIGHT_OFF = 381; // this is the servo off pulse length
const int BACK_RIGHT_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SPEED = 10;
/* --- Variables --- */

/* --- Objects --- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the maximum PWM frequency
  pwm.setPWM(FRONT_LEFT_SERVO, 0, FRONT_LEFT_OFF);
  pwm.setPWM(FRONT_RIGHT_SERVO, 0, FRONT_RIGHT_OFF);
  pwm.setPWM(BACK_LEFT_SERVO, 0, BACK_LEFT_OFF);
  pwm.setPWM(BACK_RIGHT_SERVO, 0, BACK_RIGHT_OFF);
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
  while ((center_line < LINE_THRESHOLD) && !(left_line < LINE_THRESHOLD) && !(right_line < LINE_THRESHOLD)) {
    pwm.setPWM(FRONT_LEFT_SERVO, 0, FRONT_LEFT_OFF + SERVO_SPEED );
    pwm.setPWM(FRONT_RIGHT_SERVO, 0, FRONT_RIGHT_OFF + SERVO_SPEED);
    pwm.setPWM(BACK_LEFT_SERVO, 0, BACK_LEFT_OFF + SERVO_SPEED);
    pwm.setPWM(BACK_RIGHT_SERVO, 0, BACK_RIGHT_OFF + SERVO_SPEED);
  }
  pwm.setPWM(FRONT_LEFT_SERVO, 0, FRONT_LEFT_OFF);
  pwm.setPWM(FRONT_RIGHT_SERVO, 0, FRONT_RIGHT_OFF);
  pwm.setPWM(BACK_LEFT_SERVO, 0, BACK_LEFT_OFF);
  pwm.setPWM(BACK_RIGHT_SERVO, 0, BACK_RIGHT_OFF);
}

