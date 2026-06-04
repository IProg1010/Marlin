#pragma once
#include "ch32v30x.h"
// Быстрый перевод плоского индекса Marlin (0..79) в физические структуры WCH SPL
#define PIN_TO_PORT(P)      ((GPIO_TypeDef*)(GPIOA_BASE + (((P) >> 4) * 0x0400)))
#define PIN_TO_BITMASK(P)   ((uint16_t)(1 << ((P) & 0x0F)))

// Порт A (0..15)
#define PA0  0
#define PA1  1
#define PA2  2
#define PA3  3
#define PA4  4
#define PA5  5
#define PA6  6
#define PA7  7
#define PA8  8
#define PA9  9
#define PA10 10
#define PA11 11
#define PA12 12
#define PA13 13
#define PA14 14
#define PA15 15

// Порт B (16..31)
#define PB0  16
#define PB1  17
#define PB2  18
#define PB3  19
#define PB4  20
#define PB5  21
#define PB6  22
#define PB7  23
#define PB8  24
#define PB9  25
#define PB10 26
#define PB11 27
#define PB12 28
#define PB13 29
#define PB14 30
#define PB15 31

// Порт C (32..47)
#define PC0  32
#define PC1  33
#define PC2  34
#define PC3  35
#define PC4  36
#define PC5  37
#define PC6  38
#define PC7  39
#define PC8  40
#define PC9  41
#define PC10 42
#define PC11 43
#define PC12 44
#define PC13 45
#define PC14 46
#define PC15 47

// Порт D (48..63)
#define PD0  48
#define PD1  49
#define PD2  50
#define PD3  51
#define PD4  52
#define PD5  53
#define PD6  54
#define PD7  55
#define PD8  56
#define PD9  57
#define PD10 58
#define PD11 59
#define PD12 60
#define PD13 61
#define PD14 62
#define PD15 63

// Порт E (64..79)
#define PE0  64
#define PE1  65
#define PE2  66
#define PE3  67

// Базовые макросы быстрого ввода-вывода Marlin 3.0
#define _READ(P)               ((PIN_TO_PORT(P)->INDR & PIN_TO_BITMASK(P)) ? 1 : 0)
#define _WRITE(P,V)            do { if (V) PIN_TO_PORT(P)->BSHR = PIN_TO_BITMASK(P); else PIN_TO_PORT(P)->BCR = PIN_TO_BITMASK(P); } while(0)
#define _TOGGLE(P)             do { PIN_TO_PORT(P)->OUTDR ^= PIN_TO_BITMASK(P); } while(0)

#define _SET_INPUT(P)          do { pinMode(P, INPUT); } while(0)
#define _SET_OUTPUT(P)         do { pinMode(P, OUTPUT); } while(0)

// Добавляем глобальные макросы, которые требует ядро Marlin
#define READ(P)                _READ(P)
#define WRITE(P,V)             _WRITE(P,V)
#define TOGGLE(P)              _TOGGLE(P)

#define SET_INPUT(P)           _SET_INPUT(P)
#define SET_OUTPUT(P)          _SET_OUTPUT(P)
#define SET_INPUT_PULLUP(P)    do { pinMode(P, INPUT_PULLUP); } while(0)
#define SET_INPUT_PULLDOWN(P)  do { pinMode(P, INPUT); } while(0)

// Дополнительные макросы быстрого IO для Marlin 3.0
#define OUT_WRITE(P,V)        do { _SET_OUTPUT(P); _WRITE(P,V); } while(0)

// Макросы проверки аппаратного ШИМ (пока принудительно отдаем false для MVP)
#define PWM_PIN(P)            false
#define SET_PWM(P)            do { pinMode(P, OUTPUT); } while(0)
//#define _INIT_SOFT_FAN(P)     do { pinMode(P, OUTPUT); } while(0)

#define pin_is_valid(P)    ((P) >= 0 && (P) < 80)