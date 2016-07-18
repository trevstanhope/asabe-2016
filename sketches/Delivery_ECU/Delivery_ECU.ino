/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <RunningMedian.h>

/* --- Prototypes --- */
int turn_into_transfer_zone(void);
void set_wheel_servos(int, int, int, int);
int drop_balls(void);
int up_wings(void);
int wait(void);
int follow_line(void);
int align(void);
int reverse_to_end(void);
int jump(void);
int line_detect(void);
void start_clock(void);
void stop_clock(void);
int zero(void);

/* --- Constants --- */
// Time
const int WAIT_INTERVAL = 100;
const int STEP_INTERVAL = 1000;
const int JUMP_INTERVAL = 3000;
const int TURN90_INTERVAL = 2500;

// Serial Commands
const int BAUD = 9600;
const int OUTPUT_LENGTH = 256;
const int ALIGN_COMMAND         = 'A';
const int B                     = 'B';
const int C                     = 'C';
const int DROP_BALLS_COMMAND    = 'D';
const int E                     = 'E';
const int FOLLOW_LINE_COMMAND   = 'F';
const int GREEN_COUNT_COMMAND   = 'G';
const int H                     = 'H';
const int I                     = 'I';
const int JUMP_COMMAND          = 'J';
const int K                     = 'K';
const int LINE_COMMAND          = 'L';
const int M                     = 'M';
const int N                     = 'N';
const int ORANGE_COUNT_COMMAND  = 'O';
const int P                     = 'P';
const int Q                     = 'Q';
const int REVERSE_COMMAND       = 'R';
const int SEEK_COMMAND          = 'S';
const int TURN_COMMAND          = 'T';
const int UP_COMMAND            = 'U';
const int V                     = 'V';
const int WAIT_COMMAND          = 'W';
const int X                     = 'X';
const int Y                     = 'Y';
const int ZERO_COMMAND          = 'Z';
const int UNKNOWN_COMMAND       = '?';

/// Line Threshold
const int LINE_THRESHOLD = 250; // i.e. 2.5 volts
const int OFFSET_SAMPLES = 5;

/// I/O Pins
const int LEFT_LINE_PIN = A0;
const int CENTER_LINE_PIN = A1;
const int RIGHT_LINE_PIN = A2;
// A4 - A5 reserved
const int XBEE_RX_PIN = 2;
const int XBEE_TX_PIN = 3;
const int BACKUP_SWITCH_PIN = 7;

// Channels
const int FRONT_LEFT_WHEEL_SERVO = 0;
const int FRONT_RIGHT_WHEEL_SERVO = 1;
const int BACK_LEFT_WHEEL_SERVO = 2;
const int BACK_RIGHT_WHEEL_SERVO = 3;
const int ORANGE_ARM_MICROSERVO = 4; // left
const int GREEN_ARM_MICROSERVO = 5; // right

// PWM Settings
const int GREEN_MICROSERVO_MIN = 410;
const int GREEN_MICROSERVO_MAX =  220; // this is the 'maximum' pulse length count (out of 4096)
const int ORANGE_MICROSERVO_MIN = 130; //UP position
const int ORANGE_MICROSERVO_MAX =  250; // DOWN position this is the 'maximum' pulse length count (out of 4096)
const int SERVO_MIN = 300;
const int SERVO_OFF = 335; // this is the servo off pulse length
const int SERVO_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SLOW = 10;
const int SERVO_MEDIUM = 20;
const int SERVO_FAST = 30;
const int FR = 13;
const int FL = 19;
const int BR = 21;
const int BL = 13;

// For Shield #
/* --- Variables --- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40
SoftwareSerial xbee(XBEE_RX_PIN, XBEE_TX_PIN);   // 2 = Rx  3 = Tx
RunningMedian line_offset = RunningMedian(OFFSET_SAMPLES);;
char command;
int result;
char output[OUTPUT_LENGTH];
char xbee_buffer[OUTPUT_LENGTH];
int orange_balls = 0;
int green_balls = 0;

/* --- Helper Functions --- */
// These functions are not actions, but are often called by actions
void set_wheel_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + fl + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + fr + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + bl + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + br + BR);
}
int check_switch(void) {
  return digitalRead(BACKUP_SWITCH_PIN);
}
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
void start_clock(void) {
  sprintf(xbee_buffer, "0,0,0,%d,%d,0\r\n", green_balls, orange_balls);
  xbee.println(xbee_buffer);
}
void stop_clock(void) {
  sprintf(xbee_buffer, "0,0,0,%d,%d,1\r\n", green_balls, orange_balls);
  xbee.println(xbee_buffer);
}


/* --- Setup --- */
void setup() {

  // USB
  Serial.begin(BAUD);

  // Pins
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);

  // XBEE
  xbee.begin(BAUD);

  // PWM
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + BR);
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, GREEN_MICROSERVO_MIN);
  pwm.setPWM(ORANGE_ARM_MICROSERVO, 0, ORANGE_MICROSERVO_MIN);
}

/* --- Loop --- */
// For each command specified in the *--- Prototypes ---* section, it must be specified here
// as a recognized character which is associated with an action function
void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    int value = Serial.parseInt();
    switch (command) {
      case ALIGN_COMMAND:
        result = align();
        break;
      case TURN_COMMAND:
        result = turn_into_transfer_zone();
        break;
      case FOLLOW_LINE_COMMAND:
        result = follow_line();
        break;
      case WAIT_COMMAND:
        result = wait();
        break;
      case DROP_BALLS_COMMAND:
        result = drop_balls();
        break;
      case UP_COMMAND:
        result = up_wings();
        break;
      case JUMP_COMMAND:
        result = jump();
        break;
      case ORANGE_COUNT_COMMAND:
        result = 0;
        orange_balls = value;
        break;
      case GREEN_COUNT_COMMAND:
        result = 0;
        green_balls = value;
        break;
      case LINE_COMMAND:
        result = line_detect();
        break;
      case REVERSE_COMMAND:
        result = reverse_to_end();
        break;
      case ZERO_COMMAND:
        result = zero();
        break;
      default:
        result = 255;
        command = UNKNOWN_COMMAND;
        break;
    }
    sprintf(output, "{'command':'%c','result':%d,'left':%d,'center':%d,'right':%d}", command, result, analogRead(LEFT_LINE_PIN), analogRead(CENTER_LINE_PIN), analogRead(RIGHT_LINE_PIN));
    Serial.println(output);
    Serial.flush();
  }
}

/* --- Action Functions --- */
// These functions are complete actions which are executed by sequentially by robot
// 1. Jump out of the starting zone to the 2nd line
int jump(void) {
  set_wheel_servos(SERVO_FAST, -SERVO_FAST, SERVO_FAST, -SERVO_FAST);
  delay(JUMP_INTERVAL);
  while (line_detect() == -255) {
    delay(WAIT_INTERVAL); // Drive until a line is reached
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

// 2. Align onto line by wiggling
int align(void) {
  // Rotate to the right
  set_wheel_servos(SERVO_SLOW, SERVO_SLOW, SERVO_SLOW, SERVO_SLOW);
  while (line_detect() != -255) {
    delay(WAIT_INTERVAL);
  }
  // Wiggle
  int i = 0;
  int x = line_detect();
  
  while (i <= 20) {
    if (x == 0) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i++;
    }
    else if (x == -1) {
      set_wheel_servos(SERVO_MEDIUM, -SERVO_SLOW, SERVO_MEDIUM, -SERVO_SLOW);
      i++;
    }
    else if (x == -2) {
      set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      i = 0;
    }
    else if (x == 1) {
      set_wheel_servos(SERVO_SLOW, -SERVO_MEDIUM, SERVO_SLOW, -SERVO_MEDIUM);
      i++;
    }
    else if (x == 2) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i = 0;
    }
    else if (x == -255) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i = 0;
    }
    else if (x == 255) {
      // Turn 90
      //set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      //delay(TURN90_INTERVAL);
      break;
    }
    delay(WAIT_INTERVAL);
    x = line_detect();
  }
  set_wheel_servos(0, 0, 0, 0); // Halt
  return 0;
}

// 3. Follow line to intersection
// #!TODO
int follow_line(void) {
  // Search until all line sensors are over tape (i.e. at the end)
  int x = line_detect();
  if (x == 255) {
    while (x == 255) {
      x = line_detect();
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
    }
  }
  while (true)  {
    x = line_detect();
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
      break;
    }
    else {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
    }
    delay(50);
  }
  set_wheel_servos(0, 0, 0, 0); // Stop servos
  return 0;
}

// 4. Turn and enter the transfer zone
// #!TODO
int turn_into_transfer_zone(void) {
  set_wheel_servos(SERVO_MEDIUM, -SERVO_MEDIUM, SERVO_MEDIUM, -SERVO_MEDIUM);
  delay(400);
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
  delay(2500);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

// 5. Wait for another action (e.g. transfer) to occur
int wait(void) {
  delay(WAIT_INTERVAL);
  return 0;
}

// 6. Reverse to End
// #!TODO
int reverse_to_end(void) {
  int x = line_detect();
  int skipped_intersections = 0;
  while (!check_switch()) {
    delay(50);
    x = line_detect();
    if (x == -1) {
      set_wheel_servos(-SERVO_SLOW, SERVO_MEDIUM, -SERVO_SLOW, SERVO_MEDIUM);
    }
    else if (x == -2) {
      set_wheel_servos(-SERVO_SLOW, SERVO_MEDIUM, -SERVO_SLOW, SERVO_MEDIUM);
    }
    else if (x == 1) {
      set_wheel_servos(-SERVO_MEDIUM, SERVO_SLOW, -SERVO_MEDIUM, SERVO_SLOW);
    }
    else if (x == 2) {
      set_wheel_servos(SERVO_MEDIUM, -SERVO_SLOW, SERVO_MEDIUM, -SERVO_SLOW);
    }
    else if (x == 0) {
      set_wheel_servos(-SERVO_MEDIUM, SERVO_MEDIUM, -SERVO_MEDIUM, SERVO_MEDIUM);
    }
    else if (x == -255) {
      set_wheel_servos(-SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW);
    }
    else if (x == 255) {
      set_wheel_servos(-SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW);
    }
    else {
      return x;
    }
  }
  set_wheel_servos(0, 0, 0, 0); // Halt
  return 0;
}

// 7. Drop Balls
// #!TODO
int drop_balls(void) {
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, GREEN_MICROSERVO_MAX);
  pwm.setPWM(ORANGE_ARM_MICROSERVO, 0, ORANGE_MICROSERVO_MAX);
  stop_clock();
  return 0;
}

int up_wings(void) {
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, GREEN_MICROSERVO_MIN);
  pwm.setPWM(ORANGE_ARM_MICROSERVO, 0, ORANGE_MICROSERVO_MIN);
  return 0;
}

int zero(void) {
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + BR);
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, GREEN_MICROSERVO_MIN);
  pwm.setPWM(ORANGE_ARM_MICROSERVO, 0, ORANGE_MICROSERVO_MIN);
  orange_balls = 0;
  green_balls = 0;
}

