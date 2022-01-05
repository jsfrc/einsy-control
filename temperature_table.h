#pragma once

const short temperature_table_einsy[][2] PROGMEM = {
  {23, 300}, {25, 295}, {27, 290}, {28, 285}, {31, 280}, {33, 275}, {35, 270},
  {38, 265}, {41, 260}, {44, 255}, {48, 250}, {52, 245}, {56, 240}, {61, 235},
  {66, 230}, {71, 225}, {78, 220}, {84, 215}, {92, 210}, {100, 205}, {109, 200},
  {120, 195}, {131, 190}, {143, 185}, {156, 180}, {171, 175}, {187, 170}, {205, 165},
  {224, 160}, {245, 155}, {268, 150}, {293, 145}, {320, 140}, {348, 135}, {379, 130},
  {411, 125}, {445, 120}, {480, 115}, {516, 110}, {553, 105}, {591, 100}, {628, 95},
  {665, 90}, {702, 85}, {737, 80}, {770, 75}, {801, 70}, {830, 65}, {857, 60},
  {881, 55}, {903, 50}, {922, 45}, {939, 40}, {954, 35}, {966, 30}, {977, 25},
  {985, 20}, {993, 15}, {999, 10}, {1004, 5}, {1008, 0}};

const short temperature_table_einsy_ambient[][2] PROGMEM = {
  {313, 125}, {347, 120}, {383, 115}, {422, 110}, {463, 105}, {506, 100}, {549, 95},
  {594, 90}, {638, 85}, {681, 80}, {722, 75}, {762, 70}, {799, 65}, {833, 60},
  {863, 55}, {890, 50}, {914, 45}, {934, 40}, {951, 35}, {966, 30}, {978, 25},
  {988, 20}, {996, 15}, {1002, 10}, {1007, 5}, {1012, 0}, {1015, -5}, {1017, -10},
  {1019, -15}, {1020, -20}, {1021, -25}, {1022, -30}, {1023, -35}, {1023, -40}};

const uint8_t temperature_table_einsy_len = sizeof(temperature_table_einsy) / sizeof(*temperature_table_einsy);
const uint8_t temperature_table_einsy_ambient_len = sizeof(temperature_table_einsy_ambient) / sizeof(*temperature_table_einsy_ambient);