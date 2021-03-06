#include <Arduino.h>
#include "menu_system_basics.h"
#include "hardware.h"



/*
  menu item
*/
MenuItem::MenuItem(const char* title, const Menu* leads_to):
  title(title),
  leads_to(leads_to){}


const char* MenuItem::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  size_t len = 0;
  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  return buf;
}


Menu* MenuItem::on_press(uint16_t duration){
  return nullptr;
}



/*
  menu item back
*/
const char pgmstr_back[] PROGMEM = "Back \1";
MenuItemBack::MenuItemBack() : MenuItem(pgmstr_back){}


Menu* MenuItemBack::on_press(uint16_t duration){
  current_menu->go_back();
  return nullptr;
}



/*
  menu item separator
*/
const char pgmstr_separator[] PROGMEM = "------------------";
MenuItemSeparator::MenuItemSeparator() : MenuItem(pgmstr_separator){}



/*
  menu item toggle
*/
MenuItemToggle::MenuItemToggle(bool* value, const char* title_true, const char* title_false, bool update_storage_on_change):
  MenuItem(nullptr),
  value(value),
  title_true(title_true),
  title_false(title_false),
  update_storage_on_change(update_storage_on_change){}


Menu* MenuItemToggle::on_press(uint16_t duration){
  (*value) = !(*value);
  if(update_storage_on_change) storage.save();
  return nullptr;
}


const char* MenuItemToggle::getTitle(){
  return (*value) ? title_true : title_false;
}



/*
  menu item toggle callable
*/
MenuItemToggleCallable::MenuItemToggleCallable(bool (*value_getter)(), const char* title_true, const char* title_false, void (*call_on_true)(), void (*call_on_false)(), bool update_storage_on_change):
  MenuItem(nullptr),
  value_getter(value_getter),
  title_true(title_true),
  title_false(title_false),
  call_on_true(call_on_true),
  call_on_false(call_on_false),
  update_storage_on_change(update_storage_on_change){}


Menu* MenuItemToggleCallable::on_press(uint16_t duration){
  if(value_getter()) call_on_true();
  else call_on_false();
  if(update_storage_on_change) storage.save();
  return nullptr;
}


const char* MenuItemToggleCallable::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = value_getter() ? title_true : title_false;
  size_t len = 0;
  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  return buf;
}



/*
  menu item callable
*/
MenuItemCallable::MenuItemCallable(const char* title, void (*callable)(), bool do_return):
  MenuItem(title),
  callable(callable),
  do_return(do_return){}


Menu* MenuItemCallable::on_press(uint16_t duration){
  if(callable != nullptr) callable();
  if(do_return) current_menu->go_back();
  return nullptr;
}



/*
  menu item callable w/ arg
*/
template <typename T>
MenuItemCallableArg<T>::MenuItemCallableArg(const char* title, void (*callable)(T), T value, bool do_return):
  MenuItemCallable(title, nullptr, do_return),
  callable(callable),
  value(value){}


template <typename T>
Menu* MenuItemCallableArg<T>::on_press(uint16_t duration){
  if(callable != nullptr) callable(value);
  if(do_return) current_menu->go_back();
  return nullptr;
}


// template class MenuItemCallableArg<uint32_t>;



/*
  menu item dynamic
*/
template <typename T>
MenuItemDynamic<T>::MenuItemDynamic(const char* title, T& value, const Menu* leads_to):
  MenuItem(nullptr, leads_to),
  title(title),
  value(value){}


template <typename T>
const char* MenuItemDynamic<T>::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  char buf_num[10] = {0};
  size_t len = 0;
  itoa(value, buf_num, 10);

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_num), buf_num, strlen(buf_num));

  return buf;
}


template <>
const char* MenuItemDynamic<float>::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  char buf_num[10] = {0};
  size_t len = 0;
  dtostrf(value, -8, 2, buf_num);
  for (size_t i = 0; i < sizeof(buf_num); i++) if(buf_num[i] == ' '){ buf_num[i] = 0; break; }

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_num), buf_num, strlen(buf_num));

  return buf;
}


template class MenuItemDynamic<float>;
template class MenuItemDynamic<uint16_t>;



/*
  menu item dynamic callable
*/
template <typename T>
MenuItemDynamicCallable<T>::MenuItemDynamicCallable(const char* title, T (*value_getter)()):
  MenuItem(nullptr, nullptr),
  title(title),
  value_getter(value_getter){}


template <typename T>
const char* MenuItemDynamicCallable<T>::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  char buf_num[10] = {0};
  size_t len = 0;
  T value = value_getter();
  itoa(value, buf_num, 10);

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_num), buf_num, strlen(buf_num));

  return buf;
}


template <>
const char* MenuItemDynamicCallable<float>::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  char buf_num[10] = {0};
  size_t len = 0;
  float value = value_getter();
  dtostrf(value, -8, 2, buf_num);
  for (size_t i = 0; i < sizeof(buf_num); i++) if(buf_num[i] == ' '){ buf_num[i] = 0; break; }

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_num), buf_num, strlen(buf_num));

  return buf;
}


template <>
const char* MenuItemDynamicCallable<const char*>::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  const char* buf_value = value_getter();
  size_t len = 0;

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_value), buf_value, strlen(buf_value));

  return buf;
}


template class MenuItemDynamicCallable<uint16_t>;
template class MenuItemDynamicCallable<int32_t>;
template class MenuItemDynamicCallable<float>;
template class MenuItemDynamicCallable<const char*>;



/*
  menu item dynamic time
*/
MenuItemDynamicTime::MenuItemDynamicTime(const char* title, const uint32_t* value, bool force_show_hours):
  MenuItem(nullptr, nullptr),
  title(title),
  value(value),
  force_show_hours(force_show_hours){}


const char* MenuItemDynamicTime::getTitle(){
  const uint8_t buf_len = 20;
  static char buf[buf_len];
  const char* ptr = title;
  const uint32_t sec_ = *value / 1000;
  const uint16_t mins_ = sec_ / 60;
  const uint16_t hrs = mins_ / 60;
  const uint16_t mins = mins_ % 60;
  const uint8_t sec = sec_ % 60;
  char buf_num[10] = {0};
  size_t len = 0;
  if(force_show_hours || hrs > 0){
    itoa(hrs, buf_num, 10);
    if(mins < 10){
      memcpy(buf_num + strlen(buf_num), ":0", 2);
    }else{
      memcpy(buf_num + strlen(buf_num), ":", 1);
    }
  }
  itoa(mins, buf_num + strlen(buf_num), 10);
  if(sec < 10){
    memcpy(buf_num + strlen(buf_num), ":0", 2);
  }else{
    memcpy(buf_num + strlen(buf_num), ":", 1);
  }
  itoa(sec, buf_num + strlen(buf_num), 10);

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf) - 1] = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  memcpy(buf + len - 1, ": ", 2);
  memcpy(buf + 18 - strlen(buf_num), buf_num, strlen(buf_num));

  return buf;
}



/*
  menu
*/
Menu::Menu(MenuItem* const* items, size_t items_count):
  items(items),
  items_count(items_count),
  came_from(nullptr),
  current_item(0),
  offset(0),
  redraw_interval(0){}


void Menu::on_enter(){
}


void Menu::on_leave(){
}


void Menu::on_press(uint16_t duration){
  MenuItem* item = (MenuItem*)pgm_read_word(&items[current_item]);
  Menu* new_menu = item->leads_to != nullptr ? item->leads_to : item->on_press(duration);

  if(new_menu != nullptr){
    current_menu->on_leave();
    (*new_menu).came_from = current_menu;
    new_menu->on_enter();
    current_menu = new_menu;
  }

  current_menu->draw();
}


void Menu::draw(bool clear){
  if(current_menu == nullptr) return;

  if(clear) lcd.clear();
  for (size_t i = 0; i < 4; i++) {
    uint8_t y = i + offset;
    if(y >= items_count) break;

    bool active = current_item == y;
    MenuItem* item = (MenuItem*)pgm_read_word(&items[y]);
    lcd.setCursor(0, i);
    lcd.print(active ? "\3" : " ");
    lcd.print(item->getTitle());
  }
  last_menu_redraw = millis();
}


void Menu::move(int8_t amount){
  current_item += amount;
  if(current_item < 0) current_item = 0;
  if(current_item >= items_count - 1) current_item = items_count - 1;
  int8_t y = current_item - offset;
  if(y < 1 && offset > 0) offset += y - 1;
  if(y > 2 && offset + 4 < items_count) offset += (y - 2);

  draw();
}


void Menu::loop(){
}


void Menu::go_back(){
  if(came_from != nullptr){
    on_leave();
    came_from->on_enter();
    current_menu = came_from;
    current_menu->draw();
  }
}


bool Menu::has_back(){
  for (size_t i = 0; i < items_count; i++) {
    // MenuItem* item = (MenuItem*)pgm_read_word(&items[i]);
    // if(item == back) return true;
    if((MenuItemBack*)pgm_read_word(&items[i]) == &back) return true;
  }
  return false;
}



/*
  menu range
*/
template <typename T>
MenuRange<T>::MenuRange(const char* title, T& value, T min_value, T max_value, T step, bool update_storage_on_leave):
  Menu(nullptr, 0),
  title(title),
  value(value),
  min_value(min_value),
  max_value(max_value),
  step(step),
  update_storage_on_leave(update_storage_on_leave){}


template <typename T>
void MenuRange<T>::on_enter(){
  lcd.clear();
}


template <typename T>
void MenuRange<T>::on_press(uint16_t duration){
  if(update_storage_on_leave) storage.save();
  go_back();
}


template <typename T>
void MenuRange<T>::draw(bool clear){
  const uint8_t buf_len = 18;
  const char* ptr = title;
  char buf[buf_len] = {0};
  size_t len = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  lcd.print("\3", 0, 0);
  lcd.print(buf);
  lcd.print(" \1");

  lcd.print(value > min_value ? "<" : " ", 0, 2);
  lcd.setCursor(8, 2); // we must call setCursor separately because otherwise
  lcd.print(value);    // compiler could choose wrong print function overload
  lcd.print(value < max_value ? ">" : " ", 19, 2);
}


template <typename T>
void MenuRange<T>::move(int8_t amount){
  if(value + (amount * step) > max_value) value = max_value;
  else if(value + (amount * step) < min_value) value = min_value;
  else value += (amount * step);
  // Serial.print(">");
  // Serial.println(value);
  draw();
}


template class MenuRange<int8_t>;
template class MenuRange<uint8_t>;
template class MenuRange<uint16_t>;
template class MenuRange<float>;



/*
  menu item range
*/
template <typename T>
MenuItemRange<T>::MenuItemRange(const char* title, T& value, T min_value, T max_value, T step, bool update_storage_on_leave):
  MenuItemDynamic<T>(title, value, &menu_range),
  menu_range(title, value, min_value, max_value, step, update_storage_on_leave){}


template class MenuItemRange<float>;
template class MenuItemRange<uint16_t>;


/*
  menu list
*/
template <typename T>
MenuList<T>::MenuList(const char* title, T* value, T items_list[], size_t items_count, bool update_storage_on_leave):
  Menu(nullptr, 0),
  title(title),
  value(value),
  items_count(items_count),
  index(0),
  update_storage_on_leave(update_storage_on_leave){
    items = (T*)calloc(items_count, sizeof(T*));
    if(items) memcpy(items, items_list, items_count * sizeof(T));
  }


template <typename T>
void MenuList<T>::on_enter(){
  lcd.clear();

  index = 0;
  for(size_t i = 0; i < items_count; i++) if(*value == items[i]){
    index = i;
    break;
  }
}


template <typename T>
void MenuList<T>::on_press(uint16_t duration){
  if(update_storage_on_leave) storage.save();
  go_back();
}


template <typename T>
void MenuList<T>::draw(bool clear){
  const uint8_t buf_len = 18;
  const char* ptr = title;
  char buf[buf_len] = {0};
  size_t len = 0;

  while(1){
    if((buf[len++] = pgm_read_byte(ptr++)) == 0) break;
    else if(len >= buf_len) break;
  }

  lcd.print("\3", 0, 0);
  lcd.print(buf);
  lcd.print(" \1");

  lcd.print(index > 0 ? "<" : " ", 0, 2);
  lcd.setCursor(8, 2); // we must call setCursor separately because otherwise
  lcd.print(*value);   // compiler could choose wrong print function overload
  lcd.print(index < items_count - 1 ? ">" : " ", 19, 2);
}


template <typename T>
void MenuList<T>::move(int8_t amount){
  if((int8_t)index + amount < 0) index = 0;
  else if(index + amount >= items_count) index = items_count - 1;
  else index += amount;
  *value = items[index];

  draw();
}


template class MenuList<uint8_t>;
template class MenuList<uint16_t>;




MenuItemBack back;
MenuItemSeparator separator;
Menu* current_menu = nullptr;
uint8_t last_entered_motor_menu = 0;
uint32_t last_menu_redraw = 0;
