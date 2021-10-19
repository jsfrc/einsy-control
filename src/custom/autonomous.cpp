#include "autonomous.h"
#include <Arduino.h>
#include "../../pins.h"
#include "../../menus.h"
#include "../../hardware.h"
#include "../../serial.h"
#ifdef CUSTOM_AUTONOMOUS


/// custom stuff
uint8_t half_rot_no = 20;
uint8_t rpm_start = 60;
uint8_t rpm_target = 160;
bool half_rot_dir = true;

MenuItemToggle item_half_rot_dir(&half_rot_dir, "Dir: up", "Dir: down");

MenuRange<uint8_t> menu_half_rot_no("Half rot. no.:", half_rot_no, 1, 255);
MenuItem item_half_rot_no("Half rot. no.", &menu_half_rot_no);

MenuRange<uint8_t> menu_rpm_start("RPM start:", rpm_start, 1, 255);
MenuItem item_rpm_start("RPM start", &menu_rpm_start);

MenuRange<uint8_t> menu_rpm_target("RPM target:", rpm_target, 1, 255);
MenuItem item_rpm_target("RPM target", &menu_rpm_target);


void setupCustom(){
  for (size_t i = 0; i < MOTORS_MAX; i++) {
    motors[i].driver.semax(0);
    motors[i].driver.semin(0);
    motors[i].driver.rms_current(1000);
    motors[i].driver.sgt(12);

    motors[i].driver.en_pwm_mode(false);
    motors[i].driver.pwm_autoscale(false);
    motors[i].driver.intpol(false);

    motors[i].driver.TCOOLTHRS(0);

    motors[i].microsteps(8);

    // motors[i].driver.rms_current(400);
    // motors[i].driver.sgt(6);
    // motors[i].rpm(120);
  }

  motors[3].driver.semax(5);
  motors[3].driver.semin(2);
  motors[3].driver.en_pwm_mode(true);
  motors[3].driver.pwm_autoscale(true);
  motors[3].driver.intpol(true);
  motors[3].driver.rms_current(400);
  motors[3].driver.sgt(4);
  motors[3].driver.TCOOLTHRS(460);

  // power output
  pinModeOutput(PIN_VALVE); // bed mosfet
  digitalWriteExt(PIN_VALVE, LOW);
  pinModeOutput(PIN_WATER_PUMP); // extruder mosfet
  digitalWriteExt(PIN_WATER_PUMP, LOW);


}



void do_run_rotations(){
  if(motors[0].steps_to_do || motors[1].steps_to_do){
    beep(30);
    return;
  }
  char f_buf[32] = {0};
  char cmd_buff[64] = {0};
  uint8_t i = 0;
  char* end = nullptr;

  i = dtostrf(half_rot_no / 2, -8, 2, f_buf);
  for (size_t i = 0; i < sizeof(f_buf); i++) if(f_buf[i] == ' '){
    f_buf[i] = 0;
    break;
  }

  // direction
  end = strcpy(cmd_buff, "dir x"); end += strlen(cmd_buff);
  strcpy(end, half_rot_dir ? "1 " : "0 "); end += 2;
  strcpy(end, "y"); end += strlen("y");
  strcpy(end, half_rot_dir ? "0" : "1"); end += 1;
  strcpy(end, " z"); end += strlen(" z");
  strcpy(end, half_rot_dir ? "0" : "1"); end += 1;
  processCommand(cmd_buff);

  // rpm
  processCommand(F("rpm x0.1 y0.1 z0.1"));

  // move_ramp
  end = cmd_buff;
  memset(cmd_buff, 0, sizeof(cmd_buff));
  if(half_rot_dir){
    strcpy(end, "move_ramp a300 x"); end += strlen("move_ramp a300 x");
  }else{
    strcpy(end, "move_ramp a40 x"); end += strlen("move_ramp a40 x");
  }
  strcpy(end, f_buf); end += strlen(f_buf);
  strcpy(end, " y"); end += strlen(" y");
  strcpy(end, f_buf); end += strlen(f_buf);
  strcpy(end, " z"); end += strlen(" z");
  strcpy(end, f_buf); end += strlen(f_buf);
  processCommand(cmd_buff);

  // start motors!
  processCommand(F("start x y z"));
  half_rot_dir = !half_rot_dir;

}
MenuItemCallable run_rotations("Do rotations!", &do_run_rotations, false);

void do_home_washer_linear(){
  processCommand(F("home z1 b0.5 f180 g60"));
}
MenuItemCallable item_home_washer_linear("Home washer linear", &do_home_washer_linear, false);

void do_babystep_up(){
  processCommand(F("rpm x30 y30 z30"));
  processCommand(F("dir x1 y0 z1"));
  processCommand(F("move_rot x0.05 y0.05"));
}
MenuItemCallable item_babystep_up("Babystep up", &do_babystep_up, false);

void do_babystep_down(){
  processCommand(F("rpm x30 y30 z30"));
  processCommand(F("dir x0 y1 z0"));
  processCommand(F("move_rot x0.05 y0.05"));
}
MenuItemCallable item_babystep_down("Babystep down", &do_babystep_down, false);

bool is_washing_on(){ return digitalReadExt(PIN_VALVE); }
void do_washing_on(){ digitalWriteExt(PIN_VALVE, HIGH); }
void do_washing_off(){ digitalWriteExt(PIN_VALVE, LOW); }
MenuItemToggleCallable item_washing_on_off(&is_washing_on, "Washing: on", "Washing: off", &do_washing_off, &do_washing_on);

bool is_e0_heater_on(){ return digitalReadExt(PIN_WATER_PUMP); }
void do_e0_heater_on(){ digitalWriteExt(PIN_WATER_PUMP, HIGH); }
void do_e0_heater_off(){ digitalWriteExt(PIN_WATER_PUMP, LOW); }
MenuItemToggleCallable item_e0_heater_on_off(&is_e0_heater_on, "Heater: on", "Heater: off", &do_e0_heater_off, &do_e0_heater_on);


void do_mode_stealth(){
  for (size_t i = 0; i < MOTORS_MAX; i++) {
    motors[i].driver.semax(0);
    motors[i].driver.semin(0);
    motors[i].driver.rms_current(800);
    motors[i].driver.sgt(6);

    motors[i].driver.en_pwm_mode(true);
    motors[i].driver.pwm_autoscale(true);
    motors[i].driver.intpol(true);

    motors[i].driver.TCOOLTHRS(0);

    motors[i].microsteps(8);
  }
}
MenuItemCallable item_mode_stealth("Mode: stealth", &do_mode_stealth, false);

void do_mode_normal(){
  for (size_t i = 0; i < MOTORS_MAX; i++) {
    motors[i].driver.semax(0);
    motors[i].driver.semin(0);
    motors[i].driver.rms_current(1000);
    motors[i].driver.sgt(12);

    motors[i].driver.en_pwm_mode(false);
    motors[i].driver.pwm_autoscale(false);
    motors[i].driver.intpol(false);

    motors[i].driver.TCOOLTHRS(0);

    motors[i].microsteps(8);
  }
}
MenuItemCallable item_mode_normal("Mode: normal", &do_mode_normal, false);




// main menu
MenuItem* main_menu_items[] = {
  &run_rotations,
  &item_washing_on_off,
  // &item_e0_heater_on_off,
  // &item_home_washer_linear,
  &item_half_rot_dir,
  &item_half_rot_no,
  // &item_rpm_start,
  &item_rpm_target,
  // &motor_all,
  &motor_x,
  // &motor_y,
  // &motor_z,
  &motor_e,
  // &timer1,
  // &timer2a,
  // &timer2b,
  // &timer3,
  // &timer4,
  // &timer5,
  &item_babystep_up,
  &item_babystep_down,
  &item_mode_stealth,
  &item_mode_normal,
};
Menu main_menu(main_menu_items, sizeof(main_menu_items) / 2);


#endif
