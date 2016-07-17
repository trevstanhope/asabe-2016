#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // called this way, it uses the default address 0x40
const int ARM_EXTENSION_ACTUATOR = 7;
const int PWM_FREQ = 60; // analog servos run at 60 Hz
const int ACTUATOR_MIN = 240; // retracted
const int ACTUATOR_MAX = 350; // 48mm
const int ARM_EXTENSION_DELAY = 20000;

void setup() {
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);  // This is the ideal PWM frequency for servos
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MIN);
  delay(ARM_EXTENSION_DELAY);
}

void loop() {
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MAX);
  delay(ARM_EXTENSION_DELAY);
  pwm.setPWM(ARM_EXTENSION_ACTUATOR, 0, ACTUATOR_MIN);
  delay(ARM_EXTENSION_DELAY);
}
