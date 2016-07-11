/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

/* --- Time Constants --- */
const int WAIT_INTERVAL = 100;
const int BEGIN_INTERVAL = 2000;
const int TURN45_INTERVAL = 1000;
const int TURN90_INTERVAL = 3000;
const int GRAB_INTERVAL = 1000;
const int TAP_INTERVAL = 500;
const int STEP_INTERVAL = 2400; // interval to move fully into finishing square
const int HALFSTEP_INTERVAL = 1100; // interval to move fully into finishing square

/* --- Serial / Commands --- */
const int BAUD = 9600;
const int OUTPUT_LENGTH = 256;
const int A                     = 'A';
const int B                     = 'B';
const int C                     = 'C';
const int D                     = 'D';
const int E                     = 'E';
const int F                     = 'F';
const int G                     = 'G';
const int H                     = 'H';
const int I                     = 'I';
const int J                     = 'J';
const int K                     = 'K';
const int L                     = 'L';
const int M                     = 'M';
const int N                     = 'N';
const int O                     = 'O';
const int P                     = 'P';
const int Q                     = 'Q';
const int R                     = 'R';
const int S                     = 'S';
const int TURN_COMMAND          = 'T';
const int U                     = 'U';
const int V                     = 'V';
const int WAIT_COMMAND          = 'W';
const int X                     = 'X';
const int YELLOW_GRAB_COMMAND   = 'Y';
const int Z                     = 'Z';
const int UNKNOWN_COMMAND       = '?';

/* --- Constants --- */
const int LINE_THRESHOLD = 500; // i.e. 2.5 volts
const int OFFSET_SAMPLES = 1;
const int MIN_ACTIONS = 25; // was 35

/* --- I/O Pins --- */
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;
// A4 - A5 reserved

/* --- PWM Servos --- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40

// Channels
const int FRONT_LEFT_WHEEL_SERVO = 0;
const int FRONT_RIGHT_WHEEL_SERVO = 1;
const int BACK_LEFT_WHEEL_SERVO = 2;
const int BACK_RIGHT_WHEEL_SERVO = 3;
const int YELLOW_ARM_MICROSERVO = 4;
const int GREEN_ARM_MICROSERVO = 5;

// PWM Settings
const int MICROSERVO_MIN = 150;
const int MICROSERVO_ZERO = 225; // this is the servo off pulse length
const int MICROSERVO_MAX =  600; // this is the 'maximum' pulse length count (out of 4096)
const int SERVO_MIN = 300;
const int SERVO_OFF = 381; // this is the servo off pulse length
const int SERVO_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SLOW = 20;
const int SERVO_SPEED = 15;
const int FR = -3;
const int FL = -3;
const int BR = -4;
const int BL = -3;

/* --- Variables --- */
char command;
int result;
RunningMedian offset = RunningMedian(OFFSET_SAMPLES);

/* --- Buffers --- */
char output[OUTPUT_LENGTH];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + BR);
  pwm.setPWM(SORTING_GATE_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
  pwm.setPWM(REAR_GATE_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous

}

/* --- Loop --- */
void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch (command) {
      case TURN_COMMAND:
        result = turn();
        break;
      case REPEAT_COMMAND:
        break;
      case WAIT_COMMAND:
        result = wait();
        break;
      case CLEAR_COMMAND:
        result = wait();
        break;
      default:
        result = 255;
        command = UNKNOWN_COMMAND;
        break;
    }
    sprintf(output, "{'command':'%c','result':%d}", command, result);
    Serial.println(output);
    Serial.flush();
  }
}

/* --- Actions --- */
int wait(void) {
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX);
  delay(WAIT_INTERVAL);
  return 0;
}

void set_wheel_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + fl + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + fr + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + bl + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + br + BR);
}




