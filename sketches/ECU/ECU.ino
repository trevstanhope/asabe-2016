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
const int BEGIN_COMMAND = 'B';
const int ALIGN_COMMAND  = 'A';
const int SEEK_COMMAND  = 'S';
const int END_COMMAND  = 'E';
const int GRAB_COMMAND  = 'G';
const int TURN_COMMAND  = 'T';
const int JUMP_COMMAND  = 'J';
const int FINISH_COMMAND  = 'F';
const int WAIT_COMMAND = 'W';
const int REPEAT_COMMAND = 'R';
const int CLEAR_COMMAND = 'C';
const int PING_COMMAND = 'P';
const int UNKNOWN_COMMAND = '?';

/* --- Constants --- */
const int LINE_THRESHOLD = 500; // i.e. 2.5 volts
const int DISTANCE_THRESHOLD = 36; // cm
const int FAR_THRESHOLD = 38; // cm
const int ACTIONS_PER_PLANT = 50; // was 60
const int DISTANCE_SAMPLES = 15; // was 15
const int OFFSET_SAMPLES = 1;
const int MIN_ACTIONS = 25; // was 35

/* --- I/O Pins --- */
const int LEFT_LINE_PIN = A0;
const int RIGHT_LINE_PIN = A2;
const int CENTER_LINE_PIN = A1;
const int DIST_SENSOR_PIN = 7;
// A4 - A5 reserved

/* --- PWM Servos --- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40
const int FRONT_LEFT_SERVO = 0;
const int FRONT_RIGHT_SERVO = 1;
const int BACK_LEFT_SERVO = 2;
const int BACK_RIGHT_SERVO = 3;
const int ARM_SERVO = 4;
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
int at_plant = 0; // 0: not at plant, 1-5: plant number
int at_end = 0; // 0: not at end, 1: 1st end of row, 2: 2nd end of row
int pass_num = 0; // 0: not specified, 1: right-to-left, 2: left-to-rightd
RunningMedian dist = RunningMedian(DISTANCE_SAMPLES);
RunningMedian offset = RunningMedian(OFFSET_SAMPLES);

/* --- Buffers --- */
char output[OUTPUT_LENGTH];

/* --- Setup --- */
void setup() {
  Serial.begin(BAUD);
  pinMode(CENTER_LINE_PIN, INPUT);
  pinMode(RIGHT_LINE_PIN, INPUT);
  pinMode(LEFT_LINE_PIN, INPUT);
  pinMode(DIST_SENSOR_PIN, INPUT);
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(FRONT_LEFT_SERVO, 0, SERVO_OFF + FL);
  pwm.setPWM(FRONT_RIGHT_SERVO, 0, SERVO_OFF + FR);
  pwm.setPWM(BACK_LEFT_SERVO, 0, SERVO_OFF + BL);
  pwm.setPWM(BACK_RIGHT_SERVO, 0, SERVO_OFF + BR);
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX); // it's fixed rotation, not continous
}

/* --- Loop --- */
void loop() {
  if (Serial.available() > 0) {
    char val = Serial.read();
    switch (val) {
    case BEGIN_COMMAND:
      command = BEGIN_COMMAND;
      result = begin_run();
      if (result == 0) { 
        pass_num = 1;
        at_end = 0;
        at_plant = 0; 
      }
      break;
    case ALIGN_COMMAND:
      command = ALIGN_COMMAND;
      result = align();
      if (pass_num == 1) {
        at_end = 1;
      }
      else if (pass_num == 2) {
        at_end = 2;
      }
      break;
    case SEEK_COMMAND:
      command = SEEK_COMMAND;
      result = seek_plant();
      if (result != 0) {
        at_end = 0;
        at_plant = at_plant + result;
        if (at_plant > 5) { 
          at_plant = 5; 
        }
      }
      else {
        at_plant = 0;
        if (pass_num == 1) {
          at_end = 2;
        }
        else if (pass_num == 2) {
          at_end = 1;
        }
      }
      break;
    case END_COMMAND:
      command = END_COMMAND;
      result = seek_end();
      if (result == 0) {
        at_plant = 0;
        if (pass_num == 1) {
          at_end = 2;
        }
        else if (pass_num == 2) {
          at_end = 1;
        }
      }
      break;
    case GRAB_COMMAND:
      command = GRAB_COMMAND;
      result = grab();
      break;
    case TURN_COMMAND:
      command = TURN_COMMAND;
      result = turn();
      if (result == 0) {
        at_end = 0;
        at_plant = 0;
        if (pass_num == 1) {
          pass_num = 2;
        }
      }
      break;
    case JUMP_COMMAND:
      command = JUMP_COMMAND;
      result = jump();
      if (result == 0) {
        at_end = 0;
        at_plant = 0;
        pass_num = 1;
      }
      break;
    case FINISH_COMMAND:
      command = FINISH_COMMAND;
      at_end = 0;
      at_plant = 0;
      pass_num = 0;
      result = finish_run();
      break;
    case REPEAT_COMMAND:
      break;
    case PING_COMMAND:
      command = PING_COMMAND;
      result = ping();
      break;
    case WAIT_COMMAND:
      command = WAIT_COMMAND;
      result = wait();
      break;
    case CLEAR_COMMAND:
      at_end = 0;
      at_plant = 0;
      pass_num = 0;
      command = CLEAR_COMMAND;
      result = wait();
      break;
    default:
      result = 255;
      command = UNKNOWN_COMMAND;
      break;
    }
    sprintf(output, "{'command':'%c','result':%d,'at_plant':%d,'at_end':%d,'pass_num':%d,'line':%d,'dist':%d}", command, result, at_plant, at_end, pass_num, int(offset.getMedian()), int(dist.getMedian()));
    Serial.println(output);
    Serial.flush();
  }
}

/* --- Actions --- */
int begin_run(void) {

  // Move Arm
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_ZERO);

  // Get past black square
  set_servos(4, -37, 4, -37); // Wide left sweep
  delay(3000);

  // Run until line reached
  while (abs(find_offset(LINE_THRESHOLD)) > 2) {
    delay(20);
  }

  // Pull forward
  set_servos(10, -20, 10, -20);
  delay(1000);
  
  // Rotate towards the line
  set_servos(-20, -20, -20, -20);
  while (abs(find_offset(LINE_THRESHOLD)) > 1) {
    delay(20); // Run until line reached
  }

  // Halt
  set_servos(0, 0, 0, 0);

  return 0;
}

int align(void) {
  /*
    Aligns robot at end of T.
   
   1. Wiggle onto line
   2. Reverse to end of line  
   */

  // Wiggle onto line
  int x = find_offset(LINE_THRESHOLD);
  int i = 0;
  while (i <= 20) {
    x = find_offset(LINE_THRESHOLD);
    if (x == 0) {
      set_servos(10, -10, 10, -10);
      i++;
    }
    else if (x == -1) {
      set_servos(30, 10, 30, 10);
      i++;
    }
    else if (x == -2) {
      set_servos(-40, -40, -40, -40);
      i = 0;
    }
    else if (x == 1) {
      set_servos(10, -30, 10, -30);
      i++;
    }
    else if (x == 2) {
      set_servos(-40, -40, -40, -40);
      i = 0;
    }
    else if (x == -255) {
      set_servos(-15, 15, -15, 15);
      i = 0;
    }
    else if (x == 255) {
      set_servos(15, -15, 15, -15);
      i = 0;
    }
    delay(50);
  }
  set_servos(0, 0, 0, 0); // Halt 

  // Reverse to end
  while (x != 255)  {
    x = find_offset(LINE_THRESHOLD);
    if (x == -1) {
      set_servos(-10, 20, -10, 20);
    }
    else if (x == -2) {
      set_servos(-15, -15, -15, -15);
    }
    else if (x == 1) {
      set_servos(-20, 10, -20, 10);
    }
    else if (x == 2) {
      set_servos(15, 15, 15, 15);
    }
    else if (x == 0) {
      set_servos(-10, 10, -10, 10);
    }
    else if (x == -255) {
      set_servos(0, 0, 0, 0); // Halt 
      break;
    }
  }
  
  // Pull forward onto line
  int j = 0;
  while (abs(find_offset(LINE_THRESHOLD)) > 2) {
    if (find_offset(LINE_THRESHOLD) == -255) {
      set_servos(-10, 10, -10, 10);
    }
    else {
      set_servos(10, -10, 10, -10);
    }
    find_distance();
  }
  set_servos(0, 0, 0, 0); // Halt 
  return 0;
}

int seek_plant(void) {

  // Prepare for movement
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_ZERO); 
  int x = find_offset(LINE_THRESHOLD);
  int d = find_distance();
  int actions = 0;
  
  // Move past end
  if (x == 255) {
    while (x == 255) {
      x = find_offset(LINE_THRESHOLD);
      set_servos(20, -20, 20, -20);
    }
  }
  
  // Move past plant
  while (d < FAR_THRESHOLD)  {
    d = find_distance();
    x = find_offset(LINE_THRESHOLD);
    if (x == -1) {
      set_servos(20, -10, 20, -10);
    }
    else if (x == -2) {
      set_servos(20, 20, 20, 20);
    }
    else if (x == 1) {
      set_servos(10, -20, 10, -20);
    }
    else if (x == 2) {
      set_servos(-20, -20, -20, -20);
    }
    else if (x == 0) {
      set_servos(15, -15, 15, -15);
    }
    else if (x == 255) {
      break;
    }
    else if (x == -255) {
      set_servos(10, -15, 10, -15);
    }
    delay(50);
  }

  // Search until next plant or end
  while (true) {
    x = find_offset(LINE_THRESHOLD);
    if (x == -1) {
      set_servos(20, -10, 20, -10);
      actions++;
    }
    else if (x == -2) {
      set_servos(20, 20, 20, 20);
    }
    else if (x == 1) {
      set_servos(10, -20, 10, -20);
      actions++;
    }
    else if (x == 2) {
      set_servos(-20, -20, -20, -20);
    }
    else if (x == 0) {
      set_servos(15, -15, 15, -15);
      actions++;
    }
    else if (x == -255) {
      set_servos(-10, 10, -10, 10);
      while (abs(find_offset(LINE_THRESHOLD)) > 2) {
        delay(50);
      }        
      break;
    }
    else if (x == 255) {
      set_servos(10, -10, 10, -10);
      delay(500);
      x = find_offset(LINE_THRESHOLD);
      if (x == -255) {
        set_servos(-10, 10, -10, 10);
        delay(500);
        break;
      }
    }
    if ((find_distance() <= DISTANCE_THRESHOLD) && (at_plant < 5) && (actions > MIN_ACTIONS)) {
      for (int k = 0; k<15; k++) { 
        x = find_offset(LINE_THRESHOLD);
        if (x == -1) {
          set_servos(20, -10, 20, -10);
        }
        else if (x == -2) {
          set_servos(15, 15, 15, 15);
        }
        else if (x == 1) {
          set_servos(10, -20, 10, -20);
        }
        else if (x == 2) {
          set_servos(-15, -15, -15, -15);
        }
        else if (x == 0) {
          set_servos(15, -15, 15, -15);
        }
        delay(50);
      }
      int result = round(actions / float(ACTIONS_PER_PLANT));
      if (result == 0) {
        result = 1;
      }
      set_servos(0,0,0,0);
      return result;
    }
    delay(50);
  }
  set_servos(0, 0, 0, 0); // Stop servos
  return 0;
}

int seek_end(void) {

  // Prepare for movement
  int x = find_offset(LINE_THRESHOLD);
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX); // Retract arm fully
  delay(GRAB_INTERVAL);

  // Search until end
  if (x == 255) {
    while (x == 255) {
      x = find_offset(LINE_THRESHOLD);
      set_servos(20, -20, 20, -20);
    }
  }
  while (true)  {
    x = find_offset(LINE_THRESHOLD);
    if (x == -1) {
      set_servos(30, -20, 30, -20);
    }
    else if (x == -2) {
      set_servos(20, 20, 20, 20);
    }
    else if (x == 1) {
      set_servos(20, -30, 20, -30);
    }
    else if (x == 2) {
      set_servos(-20, -20, -20, -20);
    }
    else if (x == 0) {
      set_servos(30, -30, 30, -30);
    }
    else if (x == 255) {
      set_servos(10, -10, 10, -10);
      delay(500);
      x = find_offset(LINE_THRESHOLD);
      if (x == -255) {
        set_servos(-10, 10, -10, 10);
        delay(500);
        break;
      }
    }
    delay(50);
  }
  set_servos(0, 0, 0, 0); // Stop servos
  return 0;
}

int jump(void) {
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_ZERO);
  set_servos(10, -45, 10, -45); // Wide left sweep
  delay(3000);
  while (abs(find_offset(LINE_THRESHOLD)) > 2) {
    delay(20); // Run until line reached
  }
  
  // Pull forward
  set_servos(15, -15, 15, -15);
  delay(HALFSTEP_INTERVAL);
  
  // Rotate towards the line
  set_servos(-20, -20, -20, -20);
  while (abs(find_offset(LINE_THRESHOLD)) > 2) {
    delay(20); // Run until line reached
  }
  set_servos(0,0,0,0);
  return 0;
}

int turn(void) {
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX);
  set_servos(20, -20, 20, -20);
  delay(HALFSTEP_INTERVAL);
  set_servos(20, 30, 20, 30);
  delay(TURN90_INTERVAL);
  while (abs(find_offset(LINE_THRESHOLD)) > 0) {
    set_servos(20, 30, 20, 30);
  }
  set_servos(0, 0, 0, 0);
  return 0;
}

int grab(void) {
  for (int i = MICROSERVO_MIN; i < MICROSERVO_MAX + 50; i++) {
    pwm.setPWM(ARM_SERVO, 0, i);   // Grab block
    //if (i % 2) {
      //delay(1);
    //}
  }
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX - 100 );
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX);
  delay(TAP_INTERVAL);
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_ZERO);
  delay(GRAB_INTERVAL);
  return 0;
}

int finish_run(void) {
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX);
  set_servos(20, -20, 20, -20); // move forward one space
  delay(STEP_INTERVAL);
  set_servos(27, 27, 27, 27); // turn right 90 degrees
  delay(TURN90_INTERVAL); 
  set_servos(20, -25, 20, -25); // turn right 90 degrees
  delay(HALFSTEP_INTERVAL); // was STEP_INTERVAL on K-state board
  set_servos(30, -30, 30, -30); // turn right 90 degrees
  while (find_offset(LINE_THRESHOLD) == -255) {
    delay(100);
  }
  set_servos(20, -20, 20, -20); // move forward one space
  delay(STEP_INTERVAL);
  set_servos(0, 0, 0, 0); // move forward one space
  return 0;
}

int wait(void) {
  pwm.setPWM(ARM_SERVO, 0, MICROSERVO_MAX);
  delay(WAIT_INTERVAL);
  return 0;
}

int ping(void) {
  for (int i = 0; i < DISTANCE_SAMPLES; i++) {
    find_distance();
  }
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
  else if ((l > threshold) && (c > threshold) && (r > threshold)){
    x = 255;
  }
  else if ((l > threshold) && (c < threshold) && (r > threshold)){
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

int find_distance(void) {
  int d;
  pinMode(DIST_SENSOR_PIN, OUTPUT);
  digitalWrite(DIST_SENSOR_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(DIST_SENSOR_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(DIST_SENSOR_PIN, LOW);
  pinMode(DIST_SENSOR_PIN, INPUT);
  long duration = pulseIn(DIST_SENSOR_PIN, HIGH);
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  d = duration / 29 / 2; // inches
  dist.add(d);
  int val = dist.getMedian();
  // Serial.println(val);
  return val;
}

void set_servos(int fl, int fr, int bl, int br) {
  pwm.setPWM(FRONT_LEFT_SERVO, 0, SERVO_OFF + fl + FL);
  pwm.setPWM(FRONT_RIGHT_SERVO, 0, SERVO_OFF + fr +FR);
  pwm.setPWM(BACK_LEFT_SERVO, 0, SERVO_OFF + bl + BL);
  pwm.setPWM(BACK_RIGHT_SERVO, 0, SERVO_OFF + br + BR);
}




