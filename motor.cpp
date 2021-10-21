#include "motor.h"
#include "pins.h"
#include "hardware.h"


void setupMotorTimers(){
  #if MOTORS_PRESCALER != 8
    #error MOTORS_PRESCALER is not set to 8 !!!
  #endif

  // OCR##t##A = F_CPU / MOTORS_PRESCALER / 1000;
  // TIMSK##t |= (1 << OCIE##t##A);
  // TIMSK##t = 0;
  #define SETUP_TIMER(t) \
    TCCR##t##A = 0; \
    TCCR##t##B = 0; \
    TCNT##t  = 0; \
    TCCR##t##B |= (1 << WGM##t##2); \
    TCCR##t##B |= (1 << CS##t##1);  \
    TIMSK##t = 0;

  cli();
  // setup acceleration timer
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  OCR2A = 0xFF;
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
  TIMSK2 |= (1 << OCIE2A);

  SETUP_TIMER(1);
  SETUP_TIMER(3);
  SETUP_TIMER(4);
  SETUP_TIMER(5);

  // setup diag pin interrupt
  PCICR |= (1 << PCIE2);
  PCMSK2 = 0;
  PCMSK2 |= (1 << PCINT18); // X_DIAG
  PCMSK2 |= (1 << PCINT19); // E0_DIAG
  PCMSK2 |= (1 << PCINT22); // Z_DIAG
  PCMSK2 |= (1 << PCINT23); // Y_DIAG

  // setup "wait" handling timer
  TIMSK0 |= (1 << OCIE0A);
  sei();

  #undef SETUP_TIMER

}


int8_t axis2motor(const char axis){
  for (size_t i = 0; i < MOTORS_MAX; i++) if(motors[i].axis == axis) return i;
  return -1;
}


float _rpm2rps(float rpm){
  return rpm / 60;
}


uint16_t _rps2sps(float rps, uint16_t usteps){
  return uint16_t(FSTEPS_PER_REVOLUTION * usteps * rps);
}


uint16_t _sps2ocr(uint16_t sps){
  return F_CPU / MOTORS_PRESCALER / sps;
}


uint16_t _rpm2ocr(float rpm, uint16_t usteps){
  return _rps2ocr(rpm / 60, usteps);
}


uint16_t _rps2ocr(float rps, uint16_t usteps){
  return F_CPU / MOTORS_PRESCALER / (FSTEPS_PER_REVOLUTION * usteps * rps);
}


float _ocr2rpm(uint16_t ocr, uint16_t usteps){
  return _ocr2rps(ocr, usteps) * 60;
}


float _ocr2rps(uint16_t ocr, uint16_t usteps){
  return F_CPU / MOTORS_PRESCALER / ocr / usteps / FSTEPS_PER_REVOLUTION;
}


float _rot2usteps(float rot, uint16_t usteps){
  return rot * usteps * FSTEPS_PER_REVOLUTION;
}



Motor::Motor(uint8_t step_pin, uint8_t dir_pin, uint8_t enable_pin, uint8_t cs_pin, uint8_t diag_pin,
    uint8_t* step_port, uint8_t step_bit, uint16_t* timer_compare_port, uint16_t* timer_counter_port,
    uint8_t* timer_enable_port, uint8_t timer_enable_bit, const char axis):
  axis(axis),
  step_pin(step_pin),
  dir_pin(dir_pin),
  enable_pin(enable_pin),
  cs_pin(cs_pin),
  diag_pin(diag_pin),
  step_port(step_port),
  step_bit(step_bit),
  driver(cs_pin, 0.2f),
  usteps(16),
  pause_steps(false),
  enabled(false),
  stop_on_stallguard(true),
  print_stallguard_to_serial(false),
  is_homed(false),
  position(0.0),
  inactivity_timeout(120000),
  running(false),
  stallguard_triggered(false),
  steps_to_do(0),
  steps_total(0),
  last_movement(0),
  target_rpm(-1.0),
  accel(120),
  decel(120),
  last_speed_change(last_speed_change),
  _rpm(0.0),
  timer_compare_port(timer_compare_port),
  timer_counter_port(timer_counter_port),
  timer_enable_port(timer_enable_port),
  timer_enable_bit(timer_enable_bit){
    pinModeOutput(enable_pin);
    pinModeOutput(dir_pin);
    pinModeOutput(step_pin);
    pinModeOutput(cs_pin);
    pinModeInput(diag_pin, true);
    digitalWriteExt(enable_pin, HIGH);
    digitalWriteExt(dir_pin, LOW);
    digitalWriteExt(step_pin, LOW);
    digitalWriteExt(cs_pin, HIGH);
  }


void Motor::on(){
  digitalWriteExt(enable_pin, LOW);
}


void Motor::off(){
  digitalWriteExt(enable_pin, HIGH);
  stop();
  is_homed = false;
}


bool Motor::is_on(){
  return digitalReadExt(enable_pin) == LOW;
}


void Motor::start(bool start_running){
  if(!is_on()) on();
  running = start_running;
  enabled = true;
  *timer_counter_port = 0;
  *timer_enable_port |= (1 << timer_enable_bit);
}


void Motor::stop(){
  *timer_enable_port = 0;
  enabled = false;
  running = false;
  steps_to_do = 0;
  ramp_off();
}


void Motor::step(){
  *step_port ^= 1 << step_bit;
  delayMicroseconds(2);
  *step_port ^= 1 << step_bit;
  steps_total++;
}


bool Motor::dir(){
  return digitalReadExt(dir_pin);
}


void Motor::dir(bool direction){
  digitalWriteExt(dir_pin, direction);
}


uint16_t Motor::sg_value(){
  TMC2130_n::DRV_STATUS_t drv_status{0};
  drv_status.sr = driver.DRV_STATUS();
  return drv_status.sg_result;
}


void Motor::microsteps(uint16_t microstepping){
  driver.microsteps(microstepping);
  usteps = driver.microsteps();
  // rpm(_rpm); // update timer

  if(millis() > 300){
    SERIAL_PRINT("new usteps: ");
    SERIAL_PRINTLN(usteps);
  }
}


void Motor::rpm(float value){
  _rpm = value;
  uint32_t ocr = rpm2ocr(value);
  if(ocr < 70) ocr = 70;
  if(ocr > 65535) ocr = 65535;

  if(value <= 0.0){
    stop();
    // if(millis() > 300) SERIAL_PRINTLN("low rpm - stop!");
  }

  if(*timer_compare_port != ocr ){
    // cli();
    // *timer_counter_port = 0;
    *timer_compare_port = ocr;
    // sei();

    // static uint32_t last_millis = 0;
    // uint32_t _millis = millis();
    // if(_millis > 300){
    // // if(1){
    //   SERIAL_PRINT("new ocr=");
    //   SERIAL_PRINT(ocr);
    //   last_millis = _millis;
    //   // SERIAL_PRINT("\tnew rpm=");
    //   // SERIAL_PRINT(new_rpm);
    //   SERIAL_PRINTLN();
    // }
  }
}


float Motor::rpm(){
  return _rpm;
}


void Motor::ramp_to(float value, bool keep_running){
  last_speed_change = millis();
  target_rpm = value;
  // target_rpm = ocr2rpm(rpm2ocr(value, usteps), usteps);
  // if(!running){
  //   rpm(rpm()); // update timer
  //   start(keep_running);
  // }
}


void Motor::ramp_off(){
  target_rpm = -1.0;
  last_speed_change = millis();
}


bool Motor::is_expecting_stallguard(){
  // debugPrintQueue();
  return (queue[queue_index].type == MotorQueueItemType::RUN_UNTIL_STALLGUARD);
}


MotorStallguardInfo Motor::get_stallguard_info(){
  TMC2130_n::DRV_STATUS_t drv_status{0};
  drv_status.sr = driver.DRV_STATUS();

  MotorStallguardInfo result{0};
  result.sg_result = drv_status.sg_result;
  result.fsactive = drv_status.fsactive;
  result.cs_actual = drv_status.cs_actual;
  result.rms = driver.cs2rms(drv_status.cs_actual);
  return result;
}


float Motor::rot2usteps(float rot){
  return _rot2usteps(rot, usteps);
}


uint16_t Motor::rpm2ocr(float rpm){
  return _rpm2ocr(rpm, usteps);
}


uint16_t Motor::rpm2sps(float rps){
  return uint16_t(FSTEPS_PER_REVOLUTION * usteps * rps / 60);
}


uint8_t Motor::next_queue_index(){
  return queue_index + 1 >= MOTOR_QUEUE_LEN ? 0 : queue_index + 1;
}


int16_t Motor::next_empty_queue_index(){
  uint8_t next = queue_index;
  for (size_t i = 0; i < MOTOR_QUEUE_LEN - 1; i++) {
    if(++next >= MOTOR_QUEUE_LEN) next = 0;
    if(queue[next].processed || queue[next].type == MotorQueueItemType::NOOP) return next;
  }
  return -1;
}


void Motor::set_queue_item(uint8_t index, MotorQueueItemType type, uint32_t value){
  while(index >= MOTOR_QUEUE_LEN) index -= MOTOR_QUEUE_LEN;
  queue[index].type = type;
  queue[index].value = value;
  queue[index].processed = false;
}


bool Motor::set_next_empty_queue_item(MotorQueueItemType type, uint32_t value){
  const int16_t empty = next_empty_queue_index();
  if(empty < 0) return false;
  set_queue_item(empty, type, value);
  return true;
}


void Motor::empty_queue(){
  queue_index = 0;
  memset(&queue, 0, sizeof(queue));
}


bool Motor::home(){
  Serial.println(F("Motor::home() not implemented yet"));
}


bool Motor::process_next_queue_item(bool force_ignore_wait){
  if(queue[queue_index].type == MotorQueueItemType::WAIT && !force_ignore_wait) return;

  // Serial.print(F("PNQ="));
  uint8_t next = next_queue_index();
  // Serial.println(next);
  bool process_next = false;

  if(queue[next].processed || queue[next].type == MotorQueueItemType::NOOP){
    if(running || steps_to_do > 0){
      Serial.println(F("[pnq] empty queue but motor running"));

    }else{
      stop();
      // Serial.println(F("[pnq] empty queue, stopping!"));

    }
    return false;
  }

  switch (queue[next].type) {
    case MotorQueueItemType::TURN_ON: {
      on();
      Serial.println(F("[pnq] turn on"));
      break;
    }
    case MotorQueueItemType::TURN_OFF: {
      off();
      Serial.println(F("[pnq] turn off"));
      process_next = true;
      break;
    }
    case MotorQueueItemType::STOP: {
      stop();
      Serial.println(F("[pnq] stop"));
      process_next = true;
      break;
    }
    case MotorQueueItemType::RUN_CONTINUOUS: {
      // stop_on_stallguard = false;
      start(true);
      Serial.println(F("[pnq] run cont"));
      process_next = true;
      break;
    }
    case MotorQueueItemType::RUN_UNTIL_STALLGUARD: {
      // stop_on_stallguard = true;
      start(true);
      Serial.println(F("[pnq] run until sg"));
      break;
    }
    case MotorQueueItemType::DO_STEPS: {
      steps_to_do += queue[next].value;

      Serial.print(F("[pnq] do steps "));
      Serial.println(queue[next].value);
      break;
    }
    case MotorQueueItemType::RAMP_TO: {
      // *reinterpret_cast<float*>(&queue[next].value)
      ramp_to(queue[next].value / 100.0);

      Serial.print(F("[pnq] ramp to "));
      Serial.println(queue[next].value / 100.0);
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_DIRECTION: {
      dir((bool)queue[next].value);

      Serial.print(F("[pnq] set dir "));
      Serial.println(queue[next].value);
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_RPM: {
      rpm(queue[next].value / 100.0);

      Serial.print(F("[pnq] set rpm to "));
      Serial.println(_rpm);
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_ACCEL: {
      accel = queue[next].value / 100.0;

      Serial.print(F("[pnq] set accel to "));
      Serial.println(accel);
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_DECEL: {
      decel = queue[next].value / 100.0;

      Serial.print(F("[pnq] set decel to "));
      Serial.println(decel);
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_STOP_ON_STALLGUARD: {
      stop_on_stallguard = (bool)queue[next].value;

      Serial.print(F("[pnq] set stop-on-sg to "));
      Serial.println(stop_on_stallguard ? F("true") : F("false"));
      process_next = true;
      break;
    }
    case MotorQueueItemType::SET_PRINT_STALLGUARD_TO_SERIAL: {
      print_stallguard_to_serial = (bool)queue[next].value;

      Serial.println(F("[pnq] set print sg to serial"));
      process_next = true;
      break;
    }
    case MotorQueueItemType::WAIT: {
      uint32_t _millis = millis();
      uint32_t old_val = queue[next].value;
      queue[next].value += _millis;
      queue[next].type = MotorQueueItemType::WAIT_IN_PROGRESS;

      Serial.println(F("[***] wait -> wait_in_progress"));
      Serial.print(F("[pnq] wait "));
      Serial.print(old_val);
      Serial.print(F(" (q:"));
      Serial.print(next);
      Serial.print(F(") finishes at "));
      Serial.print(queue[next].value);
      Serial.print(" now is ");
      Serial.print(_millis);
      Serial.println();
      break;
    }
    case MotorQueueItemType::BEEP: {
      beep(queue[next].value);

      Serial.println(F("[pnq] beep!"));
      process_next = true;
      break;
    }

  }
  // memset(&queue[queue_index], 0, sizeof(queue[queue_index]));
  queue[queue_index].processed = true;
  queue_index = next;
  // debugPrintQueue();
  if(queue[queue_index].type == MotorQueueItemType::WAIT || queue[queue_index].type == MotorQueueItemType::WAIT_IN_PROGRESS){
    Serial.println(F("[[pnq]] is wait or wait_in_progress:"));
    // debugPrintQueue();
  }
  // if(process_next){
  //   Serial.println(F("[pqn finished] process_next!"));
  //   process_next_queue_item();
  // }
  return true;

}


void Motor::debugPrintQueue(){
  const uint8_t next = next_queue_index();
  for (size_t i = 0; i < MOTOR_QUEUE_LEN; i++) {
    if(queue_index == i) Serial.print(F("-> "));
    else if(next == i) Serial.print(F("n> "));
    Serial.print(i);
    Serial.print(F(":\tT:"));
    Serial.print(queue[i].type);

    Serial.print(queue[i].processed ? F("\t[x]") : F("\t[ ]"));

    Serial.print(F("\tv:"));
    Serial.print(queue[i].value);
    Serial.println();
    if(queue[i].type==0 && !queue[i].processed && queue[i].value==0) break;
  }
}


void Motor::debugPrintInfo(){
  Serial.print(F("Motor: ")); Serial.println(axis);
  Serial.print(F("pause_steps = ")); Serial.println(pause_steps);
  Serial.print(F("usteps = ")); Serial.println(usteps);
  Serial.print(F("enabled = ")); Serial.println(enabled);
  Serial.print(F("dir() = ")); Serial.println(dir());
  Serial.print(F("stop_on_stallguard = ")); Serial.println(stop_on_stallguard);
  Serial.print(F("running = ")); Serial.println(running);
  Serial.print(F("steps_to_do = ")); Serial.println(steps_to_do);
  Serial.print(F("steps_total = ")); Serial.println(steps_total);
  Serial.print(F("target_rpm = ")); Serial.println(target_rpm);
  Serial.print(F("accel = ")); Serial.println(accel);
  Serial.print(F("decel = ")); Serial.println(decel);
  Serial.print(F("_rpm = ")); Serial.println(_rpm);
  Serial.print(F("queue_index = ")); Serial.println(queue_index);
}


// stepping timer
#define TIMER_ISR(t, m) ISR(TIMER##t##_COMPA_vect){  \
  if(motors[m].pause_steps) return;  \
  if(motors[m].running){  \
    motors[m].step();  \
  }else if(motors[m].steps_to_do){  \
    motors[m].steps_to_do--;  \
    motors[m].step();  \
  }else{  \
    motors[m].pause_steps = true;  \
    motors[m].process_next_queue_item();  \
    motors[m].pause_steps = false;  \
  }  \
}
TIMER_ISR(1, 0)
TIMER_ISR(3, 1)
TIMER_ISR(4, 2)
TIMER_ISR(5, 3)
#undef TIMER_ISR


// "wait" handling timer, beeper off timer
ISR(TIMER0_COMPA_vect){
  const uint32_t _millis = millis();
  readEncoder();

  for(size_t i = 0; i < MOTORS_MAX; i++){
    if(motors[i].inactivity_timeout > 0 && motors[i].last_movement > 0 &&
      _millis > motors[i].last_movement + motors[i].inactivity_timeout
    ){
      motors[i].last_movement = 0;
      motors[i].stop();
      motors[i].off();
      Serial.print(F("inactivity timeout, motor "));
      Serial.print(motors[i].axis);
      Serial.println(F(" off"));
    }

    if(motors[i].pause_steps) continue;
    uint8_t next = motors[i].next_queue_index();

    if(motors[i].queue[next].type != MotorQueueItemType::NOOP &&
      !motors[i].queue[next].processed &&
      !motors[i].queue[motors[i].queue_index].processed &&
      motors[i].queue[motors[i].queue_index].type != MotorQueueItemType::DO_STEPS &&
      motors[i].queue[motors[i].queue_index].type != MotorQueueItemType::WAIT_IN_PROGRESS &&
      motors[i].queue[motors[i].queue_index].type != MotorQueueItemType::RUN_UNTIL_STALLGUARD
    ){
      // Serial.println(F("[wait] move queue! PNQ!"));
      // motors[i].debugPrintQueue();
      motors[i].pause_steps = true;
      motors[i].process_next_queue_item();
      motors[i].pause_steps = false;
    }

    // handle WAIT_IN_PROGRESS
    if(motors[i].queue[motors[i].queue_index].type == MotorQueueItemType::WAIT_IN_PROGRESS &&
      !motors[i].queue[motors[i].queue_index].processed &&
      _millis >= motors[i].queue[motors[i].queue_index].value
    ){
      motors[i].queue[motors[i].queue_index].processed = true;
      // Serial.print(F(" wait("));
      // Serial.print(motors[i].queue_index);
      // Serial.println(F(") finished! pnq!"));
      motors[i].pause_steps = true;
      if(!motors[i].process_next_queue_item(true)){
        // motors[i].queue[motors[i].queue_index].processed = true;
        // Serial.println(F("[wait] queue is empty, marking current as processed!"));
        // Serial.println(F("[wait] queue is empty, pnq returned false!"));
      }
      motors[i].pause_steps = false;
    }

  }

  if(beeper_off_at && _millis > beeper_off_at){
    beeper_off_at = 0;
    digitalWriteExt(BEEPER, LOW);
  }
}


// acceleration handling
ISR(TIMER2_COMPA_vect){
  static uint32_t last_tick = 0;
  uint32_t _millis = millis();

  for(size_t i = 0; i < MOTORS_MAX; i++){
    if(motors[i].running || motors[i].steps_to_do){
      motors[i].last_movement = _millis;
      if(motors[i].target_rpm >= 0.0){
        float rpm = motors[i].rpm();
        float rpm_delta = motors[i].target_rpm - rpm;
        uint16_t delta_t = _millis - motors[i].last_speed_change;
        float change_fraction;

        if(rpm_delta > 0.0){
          // accelerating
          change_fraction = motors[i].accel / 1000.0 * delta_t;
          rpm += change_fraction;
          if(rpm >= motors[i].target_rpm){
            rpm = motors[i].target_rpm;
            motors[i].target_rpm = -1.0;  // stop ramping since we reached set
          }

        }else{
          // decelerating
          change_fraction = motors[i].decel / 1000.0 * delta_t;
          rpm -= change_fraction;
          if(rpm <= motors[i].target_rpm){
            rpm = motors[i].target_rpm;
            motors[i].target_rpm = -1.0;  // stop ramping since we reached set
          }

        }

        motors[i]._rpm = rpm;
        uint32_t ocr = motors[i].rpm2ocr(rpm);
        if(ocr < 70) ocr = 70;
        if(ocr > 65535) ocr = 65535;

        if(rpm <= 0.0) motors[i].stop();

        cli();
        *motors[i].timer_counter_port = 0;
        *motors[i].timer_compare_port = ocr;
        sei();

        motors[i].last_speed_change = _millis;
      }
    }
  }

  last_tick = _millis;
}


// stallguard pin change interrupt
ISR(PCINT2_vect){
  const bool sg[MOTORS_MAX] = {
    PINK & (1 << PINK2), // X_DIAG
    PINK & (1 << PINK7), // Y_DIAG
    PINK & (1 << PINK6), // Z_DIAG
    PINK & (1 << PINK3), // E0_DIAG
  };

  // Serial.println("SG Int");

  for(size_t i = 0; i < MOTORS_MAX; i++){
    if(sg[i]){
      if(motors[i].is_expecting_stallguard()){
        motors[i].running = false;
        motors[i].pause_steps = true;
        motors[i].steps_to_do = 0;
        Serial.println("[sg]");
        motors[i].process_next_queue_item();
        Serial.print("s2d=");
        Serial.println(motors[i].steps_to_do);
        motors[i].pause_steps = false;

      }else{
        motors[i].stallguard_triggered = true;
        if(motors[i].stop_on_stallguard) motors[i].stop();

      }
    }
  }

}