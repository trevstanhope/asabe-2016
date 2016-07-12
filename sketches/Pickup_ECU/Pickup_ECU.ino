/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

/* --- Prototypes --- */
int align(void);
int grab_green(void);
int grab_yellow(void);
int wait(void);
int forward_jump(int);
int forward_seek(void);
int pivot_right(int);
int pivot_left(int);
void set_wheel_servos(int, int, int, int);
int line_detect(void);
int transfer(void);
int backup(void);
int zero(void);

/* --- Constants --- */
// Time intevals
const int WAIT_INTERVAL = 100;
const int TURN45_INTERVAL = 1000;
const int TURN90_INTERVAL = 3000;
const int ARM_LIFT_DELAY = 500;
const int ARM_EXTENSION_DELAY = 100;
const int SORTING_GATE_DELAY = 500;

// Serial Commands
const int BAUD = 9600;
const int OUTPUT_LENGTH = 256;
const int ALIGN_COMMAND         = 'A';
const int BACKUP_COMMAND        = 'B';
const int C                     = 'C';
const int D                     = 'D';
const int E                     = 'E';
const int FORWARD_JUMP_COMMAND  = 'F';
const int GREEN_GRAB_COMMAND    = 'G';
const int H                     = 'H';
const int I                     = 'I';
const int J                     = 'J';
const int K                     = 'K';
const int PIVOT_LEFT_COMMAND    = 'L';
const int M                     = 'M';
const int N                     = 'N';
const int O                     = 'O';
const int P                     = 'P';
const int Q                     = 'Q';
const int PIVOT_RIGHT_COMMAND   = 'R';
const int SEEK_COMMAND          = 'S';
const int TRANSFER_COMMAND      = 'T';
const int U                     = 'U';
const int V                     = 'V';
const int WAIT_COMMAND          = 'W';
const int X                     = 'X';
const int YELLOW_GRAB_COMMAND   = 'Y';
const int ZERO_COMMAND          = 'Z';
const int UNKNOWN_COMMAND       = '?';

// Line Tracking
const int LINE_THRESHOLD = 500; // i.e. 2.5 volts
const int OFFSET_SAMPLES = 1;
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;
// A4 - A5 reserved

// Servo Channels
// Bank 1
const int FRONT_LEFT_WHEEL_SERVO = 0;
const int FRONT_RIGHT_WHEEL_SERVO = 1;
const int BACK_LEFT_WHEEL_SERVO = 2;
const int BACK_RIGHT_WHEEL_SERVO = 3;
// Bank 2
const int SORTING_GATE_MICROSERVO = 5;
const int REAR_GATE_MICROSERVO = 4;
const int ARM_LIFT_HEAVYSERVO = 6;
const int ARM_EXTENSION_ACTUATOR = 7;

// PWM Settings
// Servo limit values are the pulse length count (out of 4096)
const int MICROSERVO_MIN = 180;
const int MICROSERVO_ZERO =  300;
const int MICROSERVO_MAX =  520;
const int SERVO_MIN = 300;
const int SERVO_MAX =  460; 
const int HEAVYSERVO_MIN = 300;
const int HEAVYSERVO_MAX =  550;
const int ACTUATOR_MIN = 300;
const int ACTUATOR_MAX = 460;
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
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40

/* --- Buffers --- */
char output[OUTPUT_LENGTH];

/* --- Helper Functions --- */
int line_detect(void) {
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
  return x;
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
  pwm.setPWM(ARM_LIFT_HEAVYSERVO, 0, HEAVYSERVO_MIN);
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_MAX);
  pwm.setPWM(REAR_GATE_MICROSERVO, 0, MICROSERVO_MAX);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MAX);
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
      case PIVOT_LEFT_COMMAND:
        result = pivot_left(value);
        break;
      case ALIGN_COMMAND:
        result = align();
        break;
      case WAIT_COMMAND:
        result = wait();
        break;
      case TRANSFER_COMMAND:
        result = transfer();
        break;
      case ZERO_COMMAND:
        result = zero();
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
int grab_green(void) {
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_MAX); // Sets gate to green
  pwm.setPWM(ARM_LIFT_HEAVYSERVO, 0, HEAVYSERVO_MAX); 
  delay(ARM_LIFT_DELAY);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MAX);
  delay(ARM_EXTENSION_DELAY);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MIN);
  delay(ARM_EXTENSION_DELAY);
  pwm.setPWM(ARM_LIFT_HEAVYSERVO, 0, HEAVYSERVO_MIN); 
  delay(ARM_LIFT_DELAY);
  return 0;
}

int grab_yellow(void) {
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_MIN); // Sets gate to yellow
  pwm.setPWM(ARM_LIFT_HEAVYSERVO, 0, HEAVYSERVO_MAX); 
  delay(ARM_LIFT_DELAY);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MAX);
  delay(ARM_EXTENSION_DELAY);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MIN);
  delay(ARM_EXTENSION_DELAY);
  pwm.setPWM(ARM_LIFT_HEAVYSERVO, 0, HEAVYSERVO_MIN); 
  delay(ARM_LIFT_DELAY);
}

int wait(void) {
  delay(WAIT_INTERVAL);
  return 0;
}

int forward_jump(int value) {
  set_wheel_servos(15, -15, 15, -15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int forward_seek(void) {
  set_wheel_servos(15, -15, 15, -15);
  while (line_detect() == -255) {
    delay(20);
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int pivot_right(int value) {
  set_wheel_servos(15, 15, 15, 15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int pivot_left(int value) {
  set_wheel_servos(-15, -15, -15, -15);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int align(void) {
  /*
    Aligns robot at end of T.

    1. Wiggle onto line
    2. Reverse to end of line
  */

  // Wiggle onto line
  int x = line_detect();
  int i = 0;
  while (i <= 20) {
    x = line_detect();
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
  return 0;
}

int backup(void) { 
  
  return 0;
}

int transfer(void) {
  pwm.setPWM(REAR_GATE_MICROSERVO, 0, MICROSERVO_ZERO);
}

int zero(void) {
  setup();
  return 0;
}

