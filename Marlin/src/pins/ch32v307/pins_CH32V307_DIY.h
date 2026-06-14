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
#define LED_PIN       PA15 

// Концевики (Endstops)
#define X_MIN_PIN     PB0
#define Y_MIN_PIN     PB1
#define Z_MIN_PIN     PB12
#define X_MAX_PIN     PB13
#define Y_MAX_PIN     PB14
#define Z_MAX_PIN     PB15

// Шаговые моторы: Оси X, Y, Z
#define X_STEP_PIN    PE0
#define X_DIR_PIN     PE1
#define X_ENABLE_PIN  PE2

#define Y_STEP_PIN    PE3
#define Y_DIR_PIN     PE4
#define Y_ENABLE_PIN  PE5

#define Z_STEP_PIN    PE6
#define Z_DIR_PIN     PE7
#define Z_ENABLE_PIN  PE8

// Экструдер E0y
#define E0_STEP_PIN   PE9
#define E0_DIR_PIN    PE10
#define E0_ENABLE_PIN PE11

//SERVO
#define SERVO_1       PD12  //TIM_4_CH1_1
#define SERVO_2       PD13  //TIM_4_CH2_1
#define SERVO_3       PD14  //TIM_4_CH3_1
#define SERVO_4       PD15  //TIM_4_CH4_1

//EEPROM I2C
#define EEPROM_SCL    PB10  //I2C2_SCL
#define EEPROM_SDA    PB11  //I2C2_SDA

//ETHERNET
#define ETHERNET_LINK   PC0
#define ETHERNET_ACT    PC1
#define ETHERNET_RXP    PC6
#define ETHERNET_RXN    PC7
#define ETHERNET_TXP    PC8
#define ETHERNET_TXN    PC9

//SPI FLASH
#define FLASH_SPI_CS    PA2
#define FLASH_SPI_CLK   PA5  //SPI1_SCK
#define FLASH_SPI_DO    PA6  //SPI1_MISO
#define FLASH_SPI_DI    PA7  //I2C2_MOSI

//SWD
#define SWDIO           PA13
#define SWCLK           PA14

//USER led btn
#define BOOT            PB2
#define BTN             PB3
#define LED1            PB4
#define LED2            PA15

#define SD_SS_PIN  PB6

// Нагреватели (Heaters) и ШИМ-выходы
#define HEATER_0_PIN    PD1   // Хотэнд E0          //TIM_10_CH1_2
#define HEATER_BED_PIN  PD3  // Стол (Heated Bed)   //TIM_10_CH2_2
//#define HEATER_1_PIN    PD5   // Хотэнд E0        //TIM_10_CH3_2
//#define HEATER_BED2_PIN  PD7  // Стол (Heated Bed)//TIM_10_CH4_2
#define FAN0_PIN          PB6   //TIM_8_CH1_1
//#define FAN1_PIN        PB7   //TIM_8_CH2_1
//#define FAN2_PIN        PB8   //TIM_8_CH3_1


// Термисторы (Аналоговые входы ADC)
#define TEMP_0_PIN    0     // Канал ADC0 (PA0 или соответствующий) PA0
#define TEMP_BED_PIN  1     // Канал ADC1 PA1
//#define TEMP_1_PIN    3       // PA2
//#define TEMP_BED1_PIN  4      // PA3