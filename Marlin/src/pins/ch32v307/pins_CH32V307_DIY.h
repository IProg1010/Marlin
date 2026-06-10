/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
// {{Target: CH32V307_marlin}}
#pragma once

#ifndef HAL_PLATFORM
  #error "Wrong MCU for this board! Change your platformio.ini environment."
#endif

#define BOARD_INFO_NAME "CH32V307 DIY Board"

// Оверлорд-светодиод для тестов
#define LED_PIN       PA0 

// Концевики (Endstops)
#define X_MIN_PIN     PA5
#define Y_MIN_PIN     PA6
#define Z_MIN_PIN     PA7

// Шаговые моторы: Оси X, Y, Z
#define X_STEP_PIN    PC0
#define X_DIR_PIN     PC1
#define X_ENABLE_PIN  PC6

#define Y_STEP_PIN    PC7
#define Y_DIR_PIN     PC8
#define Y_ENABLE_PIN  PC9

#define Z_STEP_PIN    PD0
#define Z_DIR_PIN     PD1
#define Z_ENABLE_PIN  PB10

// Экструдер E0
#define E0_STEP_PIN   PB11
#define E0_DIR_PIN    PA9
#define E0_ENABLE_PIN PA10

// Нагреватели (Heaters) и ШИМ-выходы
#define HEATER_0_PIN  PB0   // Хотэнд E0
#define HEATER_BED_PIN PB1  // Стол (Heated Bed)
#define FAN0_PIN      PA4


// Термисторы (Аналоговые входы ADC)
#define TEMP_0_PIN    0     // Канал ADC0 (PA0 или соответствующий)
#define TEMP_BED_PIN  1     // Канал ADC1