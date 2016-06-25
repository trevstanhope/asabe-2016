/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <RunningMedian.h>
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
const int ALIGN_RIGHT_COMMAND   = 'A';
const int BLIND_FORWARD_COMMAND = 'B';
const int CLEAR_COMMAND         = 'C';
const int D                     = 'D';
const int E                     = 'E';
const int F                     = 'F';
const int GREEN_GRAB_COMMAND    = 'G';
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
const int REPEAT_COMMAND        = 'R';
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
const int ARM_EXTENSION_SERVO = 4;
const int SORTING_GATE_SERVO = 5;
const int REAR_GATE_SERVO = 6;

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
      case YELLOW_GRAB_COMMAND:
        result = grab_yellow();
        break;
      case GREEN_GRAB_COMMAND:
        result = grab_green();
        break;
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

int seek_end(void) {

  // Prepare for movement
  int x = find_offset(LINE_THRESHOLD);
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX); // Retract arm fully
  delay(GRAB_INTERVAL);

  // Search until end
  if (x == 255) {
    while (x == 255) {
      x = find_offset(LINE_THRESHOLD);
      set_wheel_servos(20, -20, 20, -20);
    }
  }
  while (true)  {
    x = find_offset(LINE_THRESHOLD);
    if (x == -1) {
      set_wheel_servos(30, -20, 30, -20);
    }
    else if (x == -2) {
      set_wheel_servos(20, 20, 20, 20);
    }
    else if (x == 1) {
      set_wheel_servos(20, -30, 20, -30);
    }
    else if (x == 2) {
      set_wheel_servos(-20, -20, -20, -20);
    }
    else if (x == 0) {
      set_wheel_servos(30, -30, 30, -30);
    }
    else if (x == 255) {
      set_wheel_servos(10, -10, 10, -10);
      delay(500);
      x = find_offset(LINE_THRESHOLD);
      if (x == -255) {
        set_wheel_servos(-10, 10, -10, 10);
        delay(500);
        break;
      }
    }
    delay(50);
  }
  set_wheel_servos(0, 0, 0, 0); // Stop servos
  return 0;
}

int turn(void) {
  set_wheel_servos(20, -20, 20, -20);
  delay(HALFSTEP_INTERVAL);
  set_wheel_servos(20, 30, 20, 30);
  delay(TURN90_INTERVAL);
  while (abs(find_offset(LINE_THRESHOLD)) > 0) {
    set_wheel_servos(20, 30, 20, 30);
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int grab_green(void) {
  for (int i = MICROSERVO_MIN; i < MICROSERVO_MAX + 50; i++) {
    pwm.setPWM(ARM_EXTENSION_SERVO, 0, i);   // Grab block
  }
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX - 100 );
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX);
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_ZERO);
  delay(GRAB_INTERVAL);
  return 0;
}

int grab_yellow(void) {
  for (int i = MICROSERVO_MIN; i < MICROSERVO_MAX + 50; i++) {
    pwm.setPWM(ARM_EXTENSION_SERVO, 0, i);   // Grab block
  }
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX - 100 );
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX);
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_ZERO);
  delay(GRAB_INTERVAL);
  return 0;
}

int wait(void) {
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX);
  delay(WAIT_INTERVAL);
  return 0;
}

int find_offset(int threshold) {
  int l = analogRead(LEFT_LINE_PIN);
  int c = analogRead(CENTER_LINE_PIN);
  int r = analogRead(RIGHT_LINE_PIN);
  int x;
  if ((l > threshold) && (c < threshold) && (r < threshold)) {
    x = 2; // very off
  }
  else if ((l > threshold) && (c > threshold) && (r < threshold)) {
    x = 1; // midly off
  }
  else if ((l < threshold) && (c > threshold) && (r < threshold)) {
    x = 0; // on target
  }
  else if ((l < threshold) && (c > threshold) && (r > threshold)) {
    x = -1;  // mildy off
  }
  else if ((l < threshold) && (c < threshold) && (r > threshold)) {
    x = -2; // very off
  }
  else if ((l < threshold) && (c < threshold) && (r < threshold)) {
    x = -255; // off entirely
  }
  else if ((l > threshold) && (c > threshold) && (r > threshold)) {
    x = 255;
  }
  else if ((l > threshold) && (c < threshold) && (r > threshold)) {
    x = 255;
  }
  else {
    x = 0;
  }

  offset.add(x);
  int val = offset.getMedian();
  // Serial.println(val);
  return val;
}

void set_wheel_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + fl + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + fr + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + bl + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + br + BR);
}




