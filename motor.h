#pragma once
#include "src/TMCStepper.h"
#include "custom.h"


#define FSTEPS_PER_REVOLUTION 200
#define MOTOR_QUEUE_LEN 32
#define MOTORS_PRESCALER  8
#define MOTORS_MAX  4
#define MOTOR_X 0
#define MOTOR_Y 1
#define MOTOR_Z 2
#define MOTOR_E 3

#ifndef MOTOR_DIR_0
  #define MOTOR_DIR_0 "left"
#endif

#ifndef MOTOR_DIR_1
  #define MOTOR_DIR_1 "right"
#endif

void setupMotorTimers();
int8_t axis2motor(const char);
float _rpm2rps(float);
uint32_t _rps2sps(float, uint16_t);
uint32_t _sps2ocr(uint16_t);
uint32_t _rpm2ocr(float, uint16_t);
uint32_t _rps2ocr(float, uint16_t);
float _ocr2rpm(uint16_t, uint16_t);
float _ocr2rps(uint16_t, uint16_t);
uint32_t _rot2usteps(float, uint16_t);
float _usteps2rot(uint32_t, uint16_t);


enum Bool_tristate: int8_t {
  UNSET = -1,
  FALSE = 0,
  TRUE = 1,
};


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
  SET_IS_HOMED = 17,
  SET_POSITION = 18,
  SET_POSITION_USTEPS = 19,
  RESET_STALLGUARD_TRIGGERED = 20,
  REPEAT_QUEUE = 21,
  ADD_IGNORE_STALLGUARD_STEPS = 22,
  SET_IS_HOMING = 23,
  SET_STALLGUARD_THRESHOLD = 24,
  SET_CURRENT = 25,
  SET_CURRENT_HOLD = 26,
  SET_MICROSTEPPING = 27,
  SET_HOLD_MULTIPLIER = 28,
  SET_IS_HOMED_OVERRIDE = 29,
  SET_IS_HOMING_OVERRIDE = 30,
  SET_COOLSTEP_THRESHOLD = 31,
  SET_IGNORE_STALLGUARD = 32,
};


struct MotorQueueItem {
  volatile bool processed;
  volatile MotorQueueItemType type;
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
  Motor(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, volatile uint8_t*, uint8_t, volatile uint8_t*, uint8_t, volatile uint16_t*, volatile uint16_t*, volatile uint8_t*, uint8_t, const char);
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
  bool is_busy();
  bool is_expecting_stallguard();
  MotorStallguardInfo get_stallguard_info();
  uint32_t rot2usteps(float);
  float usteps2rot(uint32_t);
  uint32_t rpm2ocr(float);
  uint32_t rpm2sps(float);
  uint32_t rps2ocr(float);
  uint32_t rps2sps(float);
  float position();
  const char axis;
  uint8_t enable_pin;
  uint8_t cs_pin;
  uint8_t diag_pin;
  volatile uint8_t* step_port;
  uint8_t step_bit;
  volatile uint8_t* dir_port;
  uint8_t dir_bit;
  TMC2130Stepper driver;
  volatile uint16_t usteps;
  volatile bool pause_steps;
  volatile bool started;
  bool invert_direction;
  volatile bool stop_on_stallguard;
  bool stop_on_stallguard_only_when_homing;
  volatile bool print_stallguard_to_serial;
  volatile bool is_homed;
  volatile bool is_homing;
  volatile Bool_tristate is_homed_override;
  volatile Bool_tristate is_homing_override;
  bool reset_is_homed_on_power_off;
  bool reset_is_homed_on_stall;
  bool sync_on_stop;
  uint16_t usteps_per_unit;
  uint32_t inactivity_timeout;
  volatile uint32_t stop_at_millis;
  volatile int32_t position_usteps;
  volatile bool running;
  volatile bool stallguard_triggered;
  volatile bool ignore_stallguard;
  volatile uint32_t steps_to_do;
  volatile uint32_t steps_total;
  volatile uint32_t ignore_stallguard_steps;
  volatile uint32_t last_movement;
  struct {
    volatile float rpm;
    volatile bool direction;
    volatile bool is_homed;
    volatile int32_t position_usteps;
    volatile float accel;
    volatile float decel;
    Motor* _parent;
    float position(){
      const bool negative = position_usteps < 0;
      const uint32_t steps = negative ? -position_usteps : position_usteps;
      return negative ? -(_parent->usteps2rot(steps)) : _parent->usteps2rot(steps);
    }
  } planned;
  struct {
    bool enabled;
    bool autohome_on_move;
    bool direction;
    float initial_rpm;
    float final_rpm;
    float initial_backstep_rot;
    float final_backstep_rot;
    float ramp_from;
    uint16_t wait_duration;
  } autohome;

  volatile float target_rpm;
  volatile float ramp_start_rpm;
  volatile uint32_t target_rpm_changed_at;
  volatile float accel;
  volatile float decel;
  volatile float accel_thousandth;
  volatile float decel_thousandth;
  float default_ramp_rpm_from;
  float default_ramp_rpm_to;
  volatile uint8_t queue_index;
  MotorQueueItem queue[MOTOR_QUEUE_LEN];
  uint8_t next_queue_index();
  int16_t next_empty_queue_index();
  void set_queue_item(uint8_t, MotorQueueItemType, uint32_t = 0);
  bool set_next_empty_queue_item(MotorQueueItemType, uint32_t = 0);
  void empty_queue();
  void sync(/*bool = false*/);
  bool process_next_queue_item(bool = false);
  void debugPrintQueue(bool = false);
  void debugPrintInfo();
  void plan_steps(uint32_t);
  void plan_rotations(float, float = 0.0);
  void plan_rotations_to(float, float = 0.0);
  void plan_home(bool, float = 120.0, float = 40.0, float = 0.1, float = 0.1, float = 0.0, uint16_t = 50);
  void plan_autohome();
  void plan_ramp_move(int32_t, float = 40.0, float = 160.0, float = 0.0, float = 0.0);
  void plan_ramp_move(float, float = 40.0, float = 160.0, float = 0.0, float = 0.0);
  void plan_ramp_move_to(int32_t, float = 40.0, float = 160.0, float = 0.0, float = 0.0);
  void plan_ramp_move_to(float, float = 40.0, float = 160.0, float = 0.0, float = 0.0);
  volatile bool _dir;
  volatile float _rpm;
  volatile uint16_t* timer_compare_port;
  volatile uint16_t* timer_counter_port;
  volatile uint8_t* timer_enable_port;
  uint8_t timer_enable_bit;
private:
  bool _process_next_queue_item(bool = false, uint8_t = 0);
  volatile bool _pnq_lock;
};



extern Motor motors[];
