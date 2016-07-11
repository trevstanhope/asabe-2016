/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

/* --- Prototypes --- */
int turn_into_transfer_zone(void);
void set_wheel_servos(int, int, int, int);
int drop_balls(void);
int wait(void);
int follow_line(void);
int align(void);
int reverse_to_end(void);
int jump(void);

/* --- Constants --- */
// Time 
const int WAIT_INTERVAL = 100;
const int JUMP_INTERVAL = 5000;

// Serial Commands
const int BAUD = 9600;
const int OUTPUT_LENGTH = 256;
const int ALIGN_COMMAND         = 'A';
const int B                     = 'B';
const int C                     = 'C';
const int DROP_BALLS_COMMAND    = 'D';
const int E                     = 'E';
const int FOLLOW_LINE_COMMAND   = 'F';
const int G                     = 'G';
const int H                     = 'H';
const int I                     = 'I';
const int JUMP_COMMAND          = 'J';
const int K                     = 'K';
const int LINE_COMMAND          = 'L';
const int M                     = 'M';
const int N                     = 'N';
const int O                     = 'O';
const int P                     = 'P';
const int Q                     = 'Q';
const int REVERSE_COMMAND       = 'R';
const int SEEK_COMMAND          = 'S';
const int TURN_COMMAND          = 'T';
const int U                     = 'U';
const int V                     = 'V';
const int WAIT_COMMAND          = 'W';
const int X                     = 'X';
const int Y                     = 'Y';
const int Z                     = 'Z';
const int UNKNOWN_COMMAND       = '?';

const int LINE_THRESHOLD = 500; // i.e. 2.5 volts
const int OFFSET_SAMPLES = 1;
const int MIN_ACTIONS = 25; // was 35

// I/O Pins
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;
// A4 - A5 reserved

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
const int SERVO_OFF = 335; // this is the servo off pulse length
const int SERVO_MAX =  460; // this is the 'maximum' pulse length count (out of 4096)
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SLOW = 10;
const int SERVO_MEDIUM = 20;
const int SERVO_FAST = 30;
const int FR = 0;
const int FL = 6;
const int BR = 3;
const int BL = 0;

/* --- Variables --- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40
char command;
int result;
char output[OUTPUT_LENGTH];

/* --- Helper Functions --- */
void set_wheel_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_WHEEL_SERVO, 0, SERVO_OFF + fl + FL);
  pwm.setPWM(FRONT_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + fr + FR);
  pwm.setPWM(BACK_LEFT_WHEEL_SERVO, 0, SERVO_OFF + bl + BL);
  pwm.setPWM(BACK_RIGHT_WHEEL_SERVO, 0, SERVO_OFF + br + BR);
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
    x = 255; // at a T
  }
  return x;
}

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
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, MICROSERVO_MIN);
  pwm.setPWM(YELLOW_ARM_MICROSERVO, 0, MICROSERVO_MAX);
}

/* --- Loop --- */
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
      case WAIT_COMMAND:
        result = wait();
        break;
      case DROP_BALLS_COMMAND:
        result = drop_balls();
        break;
      case JUMP_COMMAND:
        result = jump();
        break;
      case LINE_COMMAND:
        result = line_detect();
        break;
      case REVERSE_COMMAND:
        result = reverse_to_end();
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
  int x = line_detect();
  int i = 0;
  while (i <= 20) {
    x = line_detect();
    if (x == 0) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i++;
    }
    else if (x == -1) {
      set_wheel_servos(SERVO_MEDIUM, SERVO_SLOW, SERVO_MEDIUM, SERVO_SLOW);
      i++;
    }
    else if (x == -2) {
      set_wheel_servos(-SERVO_FAST, -SERVO_FAST, -SERVO_FAST, -SERVO_FAST);
      i = 0;
    }
    else if (x == 1) {
      set_wheel_servos(SERVO_SLOW, -SERVO_MEDIUM, SERVO_SLOW, -SERVO_MEDIUM);
      i++;
    }
    else if (x == 2) {
      set_wheel_servos(-SERVO_FAST, -SERVO_FAST, -SERVO_FAST, -SERVO_FAST);
      i = 0;
    }
    else if (x == -255) {
      set_wheel_servos(-SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW);
      i = 0;
    }
    else if (x == 255) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i = 0;
    }
    delay(50);
  }
  set_wheel_servos(0, 0, 0, 0); // Halt 
  return 0;
}

// 3. Follow line to intersection
int follow_line(void) {
  
  // Prepare for movement
  int x = line_detect();

  // Search until end
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
      set_wheel_servos(10, -10, 10, -10);
      delay(500);
      x = line_detect();
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

// 4. Turn and enter the transfer zone
int turn_into_transfer_zone(void) {
  set_wheel_servos(SERVO_SLOW, SERVO_SLOW, SERVO_SLOW, SERVO_SLOW);
  return 0;
}

// 5. Wait for another action (e.g. transfer) to occur
int wait(void) {
  delay(WAIT_INTERVAL);
  return 0;
}

// 6. Reverse to End
int reverse_to_end(void) {
  int x = line_detect();
  int skipped_intersections = 0;
  while (skipped_intersections < 2) {
    delay(WAIT_INTERVAL);
    x = line_detect();
    if (x == -1) {
      set_wheel_servos(-10, 20, -10, 20);
    }
    else if (x == -2) {
      set_wheel_servos(-SERVO_SLOW, -SERVO_SLOW, -SERVO_SLOW, -SERVO_SLOW);
    }
    else if (x == 1) {
      set_wheel_servos(-20, 10, -20, 10);
    }
    else if (x == 2) {
      set_wheel_servos(SERVO_SLOW, SERVO_SLOW, SERVO_SLOW, SERVO_SLOW);
    }
    else if (x == 0) {
      set_wheel_servos(-10, 10, -10, 10);
    }
    else if (x == -255) {
      set_wheel_servos(0, 0, 0, 0); // Halt 
      break;
    }
    else if (x == 255) {
      skipped_intersections++;
    }
  }
  return 0;
}

// 7. Drop Balls
int drop_balls(void) {
  pwm.setPWM(GREEN_ARM_MICROSERVO, 0, MICROSERVO_MAX);
  pwm.setPWM(YELLOW_ARM_MICROSERVO, 0, MICROSERVO_MIN);
  return 0;
}
