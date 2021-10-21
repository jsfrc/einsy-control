#pragma once
#include "src/TMCStepper.h"


// #define DEBUG_PRINT
#ifdef DEBUG_PRINT
  #define SERIAL_PRINT(x) Serial.print(x);
  #define SERIAL_PRINTLN(x) Serial.println(x);
#else
  #define SERIAL_PRINT(x) ;
  #define SERIAL_PRINTLN(x) ;
#endif

#define FSTEPS_PER_REVOLUTION 200
#define MOTOR_QUEUE_LEN 64
#define MOTORS_PRESCALER  8
#define MOTORS_MAX  4
#define MOTOR_X 0
#define MOTOR_Y 1
#define MOTOR_Z 2
#define MOTOR_E 3

void setupMotorTimers();
int8_t axis2motor(const char);
float _rpm2rps(float);
uint16_t _rps2sps(float, uint16_t);
uint16_t _sps2ocr(uint16_t);
uint16_t _rpm2ocr(float, uint16_t);
uint16_t _rps2ocr(float, uint16_t);
float _ocr2rpm(uint16_t, uint16_t);
float _ocr2rps(uint16_t, uint16_t);
float _rot2usteps(float, uint16_t);


enum MotorQueueItemType: uint8_t {
  NOOP = 0,
  TURN_ON = 1,
  TURN_OFF = 2,
  STOP = 3,
  RUN_CONTINUOUS = 4,
  RUN_UNTIL_STALLGUARD = 5,
  DO_STEPS = 6,
  RAMP_TO = 7,
  SET_DIRECTION = 8,
  SET_RPM = 9,
  SET_ACCEL = 10,
  SET_DECEL = 11,
  SET_STOP_ON_STALLGUARD = 12,
  SET_PRINT_STALLGUARD_TO_SERIAL = 13,
  WAIT = 14, // enqueue a delay
  WAIT_IN_PROGRESS = 15, // actual delay
  BEEP = 16,
};


struct MotorQueueItem {
  volatile bool processed;
  MotorQueueItemType type;
  uint32_t value;
};


struct MotorStallguardInfo {
  uint16_t sg_result;
  bool fsactive;
  uint8_t cs_actual;
  uint16_t rms;
};


class Motor{
public:
  Motor(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t, uint16_t*, uint16_t*, uint8_t*, uint8_t, const char);
  void on();
  void off();
  bool is_on();
  void start(bool = false);
  void stop();
  void step();
  bool dir();
  void dir(bool);
  uint16_t sg_value();
  void microsteps(uint16_t);
  void rpm(float);
  float rpm();
  void ramp_to(float, bool = false);
  void ramp_off();
  bool is_expecting_stallguard();
  MotorStallguardInfo get_stallguard_info();
  float rot2usteps(float);
  uint16_t rpm2ocr(float);
  uint16_t rpm2sps(float);
  const char axis;
  uint8_t step_pin;
  uint8_t dir_pin;
  uint8_t enable_pin;
  uint8_t cs_pin;
  uint8_t diag_pin;
  uint8_t* step_port;
  uint8_t step_bit;
  TMC2130Stepper driver;
  uint16_t usteps;
  volatile bool pause_steps;
  bool enabled;
  bool stop_on_stallguard;
  bool print_stallguard_to_serial;
  bool is_homed;
  float position;
  uint32_t inactivity_timeout;
  volatile bool running;
  volatile bool stallguard_triggered;
  volatile uint32_t steps_to_do;
  volatile uint32_t steps_total;
  volatile uint32_t last_movement;

  volatile float target_rpm;
  uint16_t accel;
  uint16_t decel;

  volatile uint32_t last_speed_change;

  volatile uint8_t queue_index;
  MotorQueueItem queue[MOTOR_QUEUE_LEN] = {0};
  uint8_t next_queue_index();
  int16_t next_empty_queue_index();
  void set_queue_item(uint8_t, MotorQueueItemType, uint32_t = 0);
  bool set_next_empty_queue_item(MotorQueueItemType, uint32_t = 0);
  void empty_queue();
  bool home();
  bool process_next_queue_item(bool = false);
  void debugPrintQueue();
  void debugPrintInfo();
// private:
  volatile float _rpm;
  uint16_t* timer_compare_port;
  uint16_t* timer_counter_port;
  uint8_t* timer_enable_port;
  uint8_t timer_enable_bit;
};



extern Motor motors[];