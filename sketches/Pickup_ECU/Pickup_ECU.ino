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
const int ALIGN_COMMAND         = 'A';
const int B                     = 'B';
const int C                     = 'C';
const int D                     = 'D';
const int E                     = 'E';
const int FORWARD_JUMP_COMMAND  = 'F';
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
const int PIVOT_RIGHT_COMMAND   = 'R';
const int SEEK_COMMAND          = 'S';
const int T                     = 'T';
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

// Servo Channels
const int FRONT_LEFT_WHEEL_SERVO = 0;
const int FRONT_RIGHT_WHEEL_SERVO = 1;
const int BACK_LEFT_WHEEL_SERVO = 2;
const int BACK_RIGHT_WHEEL_SERVO = 3;
const int ARM_EXTENSION_SERVO = 4;
const int ARM_MOTION_SERVO = 5;
const int SORTING_GATE_SERVO = 6;
const int REAR_GATE_SERVO = 7;

// PWM Settings
const int MICROSERVO_MIN = 150;
const int MICROSERVO_ZERO = 225; // this is the servo off pulse length
const int MICROSERVO_MAX =  600; // this is the 'maximum' pulse length count (out of 4096)
const int SERVO_MIN = 300;
const int SERVO_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SLOW = 20;
const int SERVO_SPEED = 15;
const int FRONT_RIGHT_ZERO = 371;
const int FRONT_LEFT_ZERO = 376;
const int BACK_RIGHT_ZERO = 371;
const int BACK_LEFT_ZERO = 375;

/* --- Variables --- */
char command;
int result;
RunningMedian offset = RunningMedian(OFFSET_SAMPLES);

/* --- Buffers --- */
char output[OUTPUT_LENGTH];

/* --- Helper Functions --- */
int find_offset(void) {
  int l = analogRead(LEFT_LINE_PIN);
  int c = analogRead(CENTER_LINE_PIN);
  int r = analogRead(RIGHT_LINE_PIN);
  int x;
  if ((l > LINE_THRESHOLD) && (c < LINE_THRESHOLD) && (r < LINE_THRESHOLD)) {
    x = 2; // very off
  }
  else if ((l > LINE_THRESHOLD) && (c > LINE_THRESHOLD) && (r < LINE_THRESHOLD)) {
    x = 1; // midly off
  }
  else if ((l < LINE_THRESHOLD) && (c > LINE_THRESHOLD) && (r < LINE_THRESHOLD)) {
    x = 0; // on target
  }
  else if ((l < LINE_THRESHOLD) && (c > LINE_THRESHOLD) && (r > LINE_THRESHOLD)) {
    x = -1;  // mildy off
  }
  else if ((l < LINE_THRESHOLD) && (c < LINE_THRESHOLD) && (r > LINE_THRESHOLD)) {
    x = -2; // very off
  }
  else if ((l < LINE_THRESHOLD) && (c < LINE_THRESHOLD) && (r < LINE_THRESHOLD)) {
    x = -255; // off entirely
  }
  else {
    x = 255;
  }

  offset.add(x);
  int val = offset.getMedian();
  // Serial.println(val);
  return val;
}

void set_wheel_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, fl + FRONT_LEFT_ZERO);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, fr + FRONT_RIGHT_ZERO);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, bl + BACK_LEFT_ZERO);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, br + BACK_RIGHT_ZERO);
}

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, FRONT_LEFT_ZERO);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, FRONT_RIGHT_ZERO);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, BACK_LEFT_ZERO);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, BACK_RIGHT_ZERO);
  pwm.setPWM(SORTING_GATE_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
  pwm.setPWM(ARM_MOTION_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
  pwm.setPWM(REAR_GATE_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous

}

/* --- Loop --- */
void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    int value = Serial.parseInt();
    switch (command) {
      case YELLOW_GRAB_COMMAND:
        result = grab_yellow();
        break;
      case GREEN_GRAB_COMMAND:
        result = grab_green();
        break;
      case FORWARD_JUMP_COMMAND:
        result = forward_jump(value);
        break;
      case SEEK_COMMAND:
        result = forward_seek();
        break;
      case PIVOT_RIGHT_COMMAND:
        result = pivot_right(value);
        break;
      case ALIGN_COMMAND:
        result = align();
        break;
      case WAIT_COMMAND:
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
// Grab Green
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

// Grab Yellow
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

// Wait
int wait(void) {
  pwm.setPWM(ARM_EXTENSION_SERVO, 0, MICROSERVO_MAX);
  delay(WAIT_INTERVAL);
  return 0;
}

int forward_jump(int value) {
  set_wheel_servos(15, -15, 15, -15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
}

int forward_seek(void) {
  set_wheel_servos(15, -15, 15, -15);
  while (find_offset() == -255) {
    delay(20);
  }
  set_wheel_servos(0, 0, 0, 0);
}

int pivot_right(int value) {
  set_wheel_servos(15, 15, 15, 15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
}

int pivot_left(int value) {
  set_wheel_servos(-15, -15, -15, -15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
}

int approach_ball(void) {}

int align(void) {
  /*
    Aligns robot at end of T.

    1. Wiggle onto line
    2. Reverse to end of line
  */

  // Wiggle onto line
  int x = find_offset();
  int i = 0;
  while (i <= 20) {
    x = find_offset();
    if (x == 0) {
      set_wheel_servos(10, -10, 10, -10);
      i++;
    }
    else if (x == -1) {
      set_wheel_servos(30, 10, 30, 10);
      i++;
    }
    else if (x == -2) {
      set_wheel_servos(-40, -40, -40, -40);
      i = 0;
    }
    else if (x == 1) {
      set_wheel_servos(10, -30, 10, -30);
      i++;
    }
    else if (x == 2) {
      set_wheel_servos(-40, -40, -40, -40);
      i = 0;
    }
    else if (x == -255) {
      set_wheel_servos(-15, 15, -15, 15);
      i = 0;
    }
    else if (x == 255) {
      set_wheel_servos(15, -15, 15, -15);
      i = 0;
    }
    delay(50);
  }
  set_wheel_servos(0, 0, 0, 0); // Halt
}

