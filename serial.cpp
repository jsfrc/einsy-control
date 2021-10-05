#include "serial.h"
#include "hardware.h"
#include "gcode.h"


uint8_t rx_buf_pos = 0;
char rx_buf[RX_BUF_LEN] = {0};
uint8_t rx_delimiter_pos[RX_PARAMS];
// extern
char rx_param[RX_PARAMS][RX_PARAM_LEN];
uint8_t rx_params;
char rx_command[RX_COMMAND_LEN];



void processCommand(const __FlashStringHelper *cmd){
  PGM_P ptr = reinterpret_cast<PGM_P>(cmd);
  char buf[RX_BUF_LEN];
  size_t len = 0;
  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
  }
  processCommand(buf, len - 1);
}



void processCommand(const char *cmd, size_t len){
  bool ok = true;
  rx_params = 0;
  memset(rx_command, 0, RX_COMMAND_LEN);
  memset(rx_param, 0, RX_PARAMS * RX_PARAM_LEN);
  for(size_t i = 0; i < len; i++){
    if(cmd[i] == ' '){
      rx_delimiter_pos[rx_params] = i;
      if(rx_params < RX_PARAMS){
        rx_params++;
      }else{
        Serial.println(F("max cmd params reached!"));
      }
    }
  }

  if(rx_params > 0){
    memcpy(rx_command, cmd, rx_delimiter_pos[0]);
    for(size_t i = 0; i < rx_params; i++){
      if(i == rx_params - 1){ // last
        memcpy(rx_param[i], cmd + rx_delimiter_pos[i] + 1, len - rx_delimiter_pos[i] - 1);
      }else{
        memcpy(rx_param[i], cmd + rx_delimiter_pos[i] + 1, rx_delimiter_pos[i + 1] - rx_delimiter_pos[i] - 1);
      }
    }

  }else{
    memcpy(rx_command, cmd, len);
  }

  strToLower(rx_command);
  if(strcmp_P(rx_command, F("on"))) gcode_on();
  else if(strcmp_P(rx_command, F("off"))) gcode_off();
  else if(strcmp_P(rx_command, F("start"))) gcode_start();
  else if(strcmp_P(rx_command, F("stop"))) gcode_stop();
  else if(strcmp_P(rx_command, F("halt"))) gcode_halt();
  else if(strcmp_P(rx_command, F("rpm"))) gcode_rpm();
  else if(strcmp_P(rx_command, F("accel"))) gcode_accel();
  else if(strcmp_P(rx_command, F("decel"))) gcode_decel();
  else if(strcmp_P(rx_command, F("ramp")) || strcmp_P(rx_command, F("ramp_to"))) gcode_ramp_to();
  else if(strcmp_P(rx_command, F("move_rot"))) gcode_move_rot();
  else if(strcmp_P(rx_command, F("home"))) gcode_home();
  else if(strcmp_P(rx_command, F("print_queue"))) gcode_print_queue();
  else if(strcmp_P(rx_command, F("empty_queue"))) gcode_empty_queue();
  else if(strcmp_P(rx_command, F("wait_for_motor"))) gcode_wait_for_motor();
  else{
    ok = false;
    Serial.print(F("unknown command "));
    Serial.println(rx_command);
  }


  if(ok) Serial.println("ok");

}



void handleSerial(){
  while(Serial.available()){
    const char ch = Serial.read();
    rx_buf[rx_buf_pos++] = ch;

    if(ch == '\n'){
      rx_buf[--rx_buf_pos] = 0;
      if(rx_buf[rx_buf_pos - 1] == '\r') rx_buf[--rx_buf_pos] = 0;
      processCommand(rx_buf, rx_buf_pos);
      rx_buf_pos = 0;
      break;
    }
  }
}



bool strcmp_P(const char *s1, const __FlashStringHelper *s2){
  PGM_P ptr = reinterpret_cast<PGM_P>(s2);
  char buf[RX_BUF_LEN];
  size_t len = 0;
  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
  }
  return strcmp(s1, buf) == 0;
}



void strToLower(char *str){
  const uint8_t len = strlen(str);
  for (size_t i = 0; i < len; i++) if(65 <= str[i] && str[i] <= 90) str[i] += 32;
}



void strToUpper(char *str){
  const uint8_t len = strlen(str);
  for (size_t i = 0; i < len; i++) if(97 <= str[i] && str[i] <= 122) str[i] -= 32;
}
