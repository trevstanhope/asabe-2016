/* --- Libraries --- */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

/* --- Prototypes --- */
int align(void);
int grab_green(void);
int grab_orange(void);
int wait(void);
int forward(int);
int seek_line(void);
int pivot_right(int);
int pivot_left(int);
void set_wheel_servos(int, int, int, int);
int line_detect(void);
int transfer(void);
int backup(int value);
int zero(void);
int center_manuever(void);
int edge_manuever(void);
int jump(void);
int check_switch(void);

/* --- Constants --- */
// Time intevals
const int WAIT_INTERVAL = 100;
const int TURN30_INTERVAL = 2000;
const int TURN60_INTERVAL = 3000;
const int TURN90_INTERVAL = 3500;
const int TURN135_INTERVAL = 4000;
const int SWEEP45_INTERVAL = 3500;
const int SWEEP90_INTERVAL = 5000;
const int ARM_LIFT_DELAY = 500;
const int ARM_EXTENSION_DELAY = 8000;
const int SORTING_GATE_DELAY = 500;

// Serial Commands
const int BAUD = 9600;
const int OUTPUT_LENGTH = 256;
const int ALIGN_COMMAND         = 'A';
const int BACKUP_COMMAND        = 'B';
const int CENTER_COMMAND        = 'C';
const int D                     = 'D';
const int EDGE_COMMAND          = 'E';
const int FORWARD_COMMAND       = 'F';
const int GREEN_GRAB_COMMAND    = 'G';
const int H                     = 'H';
const int I                     = 'I';
const int JUMP_COMMAND          = 'J';
const int K                     = 'K';
const int PIVOT_LEFT_COMMAND    = 'L';
const int M                     = 'M';
const int N                     = 'N';
const int ORANGE_GRAB_COMMAND   = 'O';
const int P                     = 'P';
const int Q                     = 'Q';
const int PIVOT_RIGHT_COMMAND   = 'R';
const int SEEK_COMMAND          = 'S';
const int TRANSFER_COMMAND      = 'T';
const int U                     = 'U';
const int V                     = 'V';
const int WAIT_COMMAND          = 'W';
const int X                     = 'X';
const int Y                     = 'Y';
const int ZERO_COMMAND          = 'Z';
const int UNKNOWN_COMMAND       = '?';

/// I/O Pins
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;
// const int A4_RESERVED = A4;
// const int A5_RESERVED = A5;
const int BACKUP_SWITCH_PIN = 7;

/// Servo Channels
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

// Line Tracking
const int LINE_THRESHOLD = 500; // i.e. 2.5 volts
const int OFFSET_SAMPLES = 1;

/// PWM Settings
// Servo limit values are the pulse length count (out of 4096)
const int MICROSERVO_SORTING_GATE_MIN = 470;
const int MICROSERVO_SORTING_GATE_MAX = 200;
const int MICROSERVO_MIN = 170;
const int MICROSERVO_ZERO =  300;
const int MICROSERVO_MAX =  520;
const int GATE_MICROSERVO_CLOSED = 250;
const int GATE_MICROSERVO_OPEN = 570;
const int GATE_MICROSERVO_OFF = 0;
const int SERVO_MIN = 300;
const int SERVO_MAX =  460;
const int HEAVYSERVO_MIN = 350;//was 250
const int HEAVYSERVO_MAX =  550; // perfect
const int ACTUATOR_MIN = 240;
const int ACTUATOR_MAX = 330;
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int SERVO_SLOW = 10;
const int SERVO_MEDIUM = 20;
const int SERVO_FAST = 40;
const int FRONT_RIGHT_ZERO = 371;
const int FRONT_LEFT_ZERO = 376;
const int BACK_RIGHT_ZERO = 370;
const int BACK_LEFT_ZERO = 375;
const int BACKUP_CORRECTION = 5;

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
int check_switch(void) {
  return digitalRead(BACKUP_SWITCH_PIN);
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
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_SORTING_GATE_MAX);
  pwm.setPWM(REAR_GATE_MICROSERVO, 0, GATE_MICROSERVO_CLOSED);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MIN);
}

/* --- Loop --- */
void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    int value = Serial.parseInt();
    switch (command) {
      case ORANGE_GRAB_COMMAND:
        result = grab_orange();
        break;
      case GREEN_GRAB_COMMAND:
        result = grab_green();
        break;
      case FORWARD_COMMAND:
        result = forward(value);
        break;
      case BACKUP_COMMAND:
        result = backup(value);
        break;
      case SEEK_COMMAND:
        result = seek_line();
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
      case EDGE_COMMAND:
        result = edge_manuever();
        break;
      case CENTER_COMMAND:
        result = center_manuever();
        break;
      case JUMP_COMMAND:
        result = jump();
        break;
      default:
        result = 255;
        command = UNKNOWN_COMMAND;
        break;
    }
    sprintf(output, "{'command':'%c','result':%d, 'line':%d}", command, result, line_detect());
    Serial.println(output);
    Serial.flush();
  }
}

/* --- Actions --- */
int grab_green(void) {
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_SORTING_GATE_MAX); // Sets gate to green
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

int grab_orange(void) {
  pwm.setPWM(SORTING_GATE_MICROSERVO, 0, MICROSERVO_SORTING_GATE_MIN); // Sets gate to yellow
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

int forward(int value) {
  set_wheel_servos(SERVO_MEDIUM, -SERVO_MEDIUM, SERVO_MEDIUM, -SERVO_MEDIUM);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int seek_line(void) {
  set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
  delay(TURN135_INTERVAL);
  set_wheel_servos(SERVO_MEDIUM, -SERVO_MEDIUM, SERVO_MEDIUM, -SERVO_MEDIUM);
  while (line_detect() == -255) {
    delay(20);
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int pivot_right(int value) {
  set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int pivot_left(int value) {
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
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
  // Advance a small distance
  set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
  delay(1000);
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
  while (true) {
    if (line_detect() == 0) {
      break;
    }
    delay(20);
  }
  int x = line_detect();
  int i = 0;
  while (i <= 5) {
    x = line_detect();
    if (x == 0) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i++;
    }
    else if (x == -1) {
      set_wheel_servos(SERVO_FAST, -SERVO_SLOW, SERVO_FAST, -SERVO_SLOW);
      i++;
    }
    else if (x == -2) {
      set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      i = 0;
    }
    else if (x == 1) {
      set_wheel_servos(SERVO_SLOW, -SERVO_FAST, SERVO_SLOW, -SERVO_FAST);
      i++;
    }
    else if (x == 2) {
      set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
      i = 0;
    }
    else if (x == -255) {
      set_wheel_servos(-SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW);
    }
    else if (x == 255) {
      while (line_detect() != 0) {
        set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      }
    }
    delay(WAIT_INTERVAL);
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int backup(int value) {
  set_wheel_servos(-(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_MEDIUM, -(SERVO_SLOW + BACKUP_CORRECTION), SERVO_SLOW);
  delay(value);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int transfer(void) {
  /*
    Aligns robot at end of T.
    1. Wiggle onto line
    2. Reverse to end of line
  */
  // Advance a small distance
  set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
  delay(500);
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
  while (true) {
    if (line_detect() == 0) {
      break;
    }
    delay(20);
  }
  int x = line_detect();
  int i = 0;
  while (i <= 5) {
    x = line_detect();
    if (x == 0) {
      set_wheel_servos(SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW);
      i++;
    }
    else if (x == -1) {
      set_wheel_servos(SERVO_FAST, -SERVO_SLOW, SERVO_FAST, -SERVO_SLOW);
      i++;
    }
    else if (x == -2) {
      set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      i = 0;
    }
    else if (x == 1) {
      set_wheel_servos(SERVO_SLOW, -SERVO_FAST, SERVO_SLOW, -SERVO_FAST);
      i++;
    }
    else if (x == 2) {
      set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
      i = 0;
    }
    else if (x == -255) {
      set_wheel_servos(-SERVO_SLOW, SERVO_SLOW, -SERVO_SLOW, SERVO_SLOW);
    }
    else if (x == 255) {
      while (line_detect() != 0) {
        set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
      }
    }
    delay(WAIT_INTERVAL);
  }
  set_wheel_servos(0, 0, 0, 0);

  // Backup
  while (!check_switch())  {
    x = line_detect();
    if (x == -2) {
      set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
    }
    else if (x == -1) {
      set_wheel_servos(-(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_SLOW, -(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_SLOW);      
    }
    else if (x == 0) {
      set_wheel_servos(-(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_MEDIUM, -(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_MEDIUM);      
    }
    else if (x == 1) {
      set_wheel_servos(-(SERVO_SLOW + BACKUP_CORRECTION), SERVO_MEDIUM, -(SERVO_SLOW + BACKUP_CORRECTION), SERVO_MEDIUM);
    }
    else if (x == 2) {
      set_wheel_servos(SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM, SERVO_MEDIUM);
    }
    else if (x == 255) {
      set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
    }
    else if (x == -255) {
      set_wheel_servos(-(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_MEDIUM, -(SERVO_MEDIUM + BACKUP_CORRECTION), SERVO_MEDIUM);
    }
  }
  set_wheel_servos(0, 0, 0, 0); // Stop servos
  pwm.setPWM(REAR_GATE_MICROSERVO, 0, GATE_MICROSERVO_OPEN);
  delay(WAIT_INTERVAL);
  pwm.setPWM(REAR_GATE_MICROSERVO, 0, GATE_MICROSERVO_OFF);
  return 0;
}

int zero(void) {
  setup();
  return 0;
}

int center_manuever(void) {
  set_wheel_servos(-SERVO_SLOW, SERVO_FAST, -SERVO_SLOW, SERVO_FAST);
  delay(SWEEP45_INTERVAL);
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
  delay(TURN60_INTERVAL);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

int edge_manuever(void) {
  set_wheel_servos(-SERVO_SLOW, SERVO_FAST, -SERVO_SLOW, SERVO_FAST);
  delay(SWEEP90_INTERVAL);
  set_wheel_servos(-SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM, -SERVO_MEDIUM);
  delay(TURN90_INTERVAL);
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}


int jump(void) {
  set_wheel_servos(SERVO_MEDIUM, -SERVO_SLOW, SERVO_MEDIUM, -SERVO_SLOW);
  delay(TURN30_INTERVAL);
  set_wheel_servos(SERVO_MEDIUM, -SERVO_MEDIUM, SERVO_MEDIUM, -SERVO_MEDIUM);
  while (line_detect() == -255) {
    delay(20);
  }
  set_wheel_servos(0, 0, 0, 0);
  return 0;
}

