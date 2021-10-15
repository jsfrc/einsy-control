#include "pins.h"
#include "hardware.h"
#include "serial.h"
#include "menu_system_derivates.h"
#include "menus.h"
#include "custom.h"


uint16_t print_stallguard_to_serial_interval = 30;
uint32_t last_stallguard_print_to_serial = 0;
uint32_t last_lcd_reinit = 0;


uint16_t counter = 0;


void setup() {
  Serial.begin(250000);
  Serial.setTimeout(100);
  Serial.println(F("init..."));
  #ifdef CUSTOM
    Serial.println(F("custom_variant: " CUSTOM));
  #endif

  setupPins();
  SPI.begin();

  setupLcd();
  setupMotors();

  current_menu = &main_menu;
  current_menu->draw();
  menu_motor_stallguard_value.redraw_interval = 50;

  #ifdef CUSTOM
    setupCustom();
  #endif

  Serial.println(F("ready!"));

}


void loop() {
  uint32_t _millis = millis();

  static uint32_t last_report = 0;
  if(_millis > last_report + 1000){
    uint16_t cnt = counter;
    counter = 0;
    if(cnt){
      Serial.print(cnt);
      Serial.print("\t");
      Serial.println(1000.0 / cnt);
    }
    last_report = _millis;
  }

  static bool rpm_report = false;
  static uint32_t last_rpm_report = 0;
  if(rpm_report && (_millis > last_rpm_report + 50)){
    if(motors[0].steps_to_do || motors[0].running){
      Serial.println(motors[0]._rpm);
    }
    last_rpm_report = _millis;
  }

  readEncoder();

  current_menu->loop();

  if(enc_diff){
    current_menu->move(enc_diff);
    enc_diff = 0;

  }

  if(enc_click){
    current_menu->on_press(enc_click);
    enc_click = 0;

  }

  if(current_menu->redraw_interval > 0 && _millis > last_menu_redraw + current_menu->redraw_interval) current_menu->draw(false);

  handleSerial();

  if(_millis >= last_stallguard_print_to_serial + print_stallguard_to_serial_interval){
    bool printed_something = false;
    for(size_t i = 0; i < MOTORS_MAX; i++){
      if(motors[i].print_stallguard_to_serial){
        printed_something = true;
        MotorStallguardInfo stallguard_info = motors[i].get_stallguard_info();
        // Serial.print("M");
        // Serial.print(i);
        // Serial.print("\t");
        Serial.print(stallguard_info.sg_result);
        Serial.print("\t");
        // Serial.print(stallguard_info.fsactive);
        // Serial.print("\t");
        // Serial.print(stallguard_info.cs_actual);
        // Serial.print("\t");
        Serial.print(stallguard_info.rms);
        Serial.print("mA\t");
        // Serial.println();
      }
    }
    if(printed_something) Serial.println();
    last_stallguard_print_to_serial = _millis;
  }


  for(size_t i = 0; i < MOTORS_MAX; i++){
    if(motors[i].stallguard_triggered){
      motors[i].stallguard_triggered = false;
      const float rpm = motors[i].rpm();
      Serial.print("Motor ");
      Serial.print("XYZE"[i]);
      Serial.print(" stalled at ");
      Serial.print(rpm);
      Serial.print(" RPM!");
      Serial.println();

    }

  }

  if(_millis > last_lcd_reinit + 5000){
    lcd.reinit();
    current_menu->draw();
    last_lcd_reinit = _millis;
  }

  #ifdef CUSTOM
    loopCustom();
  #endif

}
