#pragma once
#include "serial.h"


void gcode_on();
void gcode_off();
void gcode_start();
void gcode_stop();
void gcode_halt();
void gcode_run();
void gcode_rpm();
void gcode_dir();
void gcode_accel();
void gcode_decel();
void gcode_ramp_to();
void gcode_do_steps();
void gcode_do_steps_dir();
void gcode_move_rot();
void gcode_move_rot_to();
void gcode_move_ramp();
void gcode_move_ramp_to();
void gcode_move();
void gcode_home();
void gcode_autohome();
void gcode_print_queue();
void gcode_empty_queue();
void gcode_print_info();
void gcode_pos_usteps();
void gcode_pos();
void gcode_stop_on_stallguard();
void gcode_print_stallguard();
void gcode_wait_for_motor();
void gcode_wait();
void gcode_beep();
void gcode_repeat_queue();
void gcode_set_position();
void gcode_set_invert_direction();
void gcode_reset_steps_total();
void gcode_is_busy();
void gcode_is_homed();
void gcode_is_homing();
void gcode_test_sg();
