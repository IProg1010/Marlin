/*#pragma once

#include <stdint.h>
#include <stddef.h>

// Подключаем заголовочные файлы производителя, которые скачал PlatformIO
#include "ch32v30x.h"

// Базовые типы данных, которые Marlin требует от архитектуры
typedef uint32_t hal_timer_t;
#define HAL_TIMER_TYPE_MAX 0xFFFFFFFF

typedef int8_t pin_t;

// Заглушки для критических секций (отключение прерываний)
#define CRITICAL_SECTION_START() uint32_t primask = __get_PRIMASK(); __disable_irq()
#define CRITICAL_SECTION_END()   __set_PRIMASK(primask)

// Макросы для быстрого IO (пока пустые, чтобы просто скомпилировать)
#define SET_INPUT(P)        
#define SET_OUTPUT(P)       
#define WRITE(P,V)          
#define READ(P)             0
#define TOGGLE(P)           

// Обязательные функции, которые Marlin будет вызывать
void HAL_init();
void idletask();

inline void HAL_clear_reset_source() {}
inline uint8_t HAL_get_reset_source() { return 0; }
inline void HAL_reboot() { NVIC_SystemReset(); }
*/
#pragma once

#ifndef HAL_PLATFORM_CH32V307
  #define HAL_PLATFORM_CH32V307
#endif

#include <stdint.h>
#include <stddef.h>
#include "ch32v30x.h"

// Константы Arduino API
#define HIGH 0x1
#define LOW  0x0
#define INPUT        0x0
#define OUTPUT       0x1
#define INPUT_PULLUP 0x2

// Макросы прогмем (для AVR совместимости строк, на 32 битах они пустые)
#define PROGMEM
#define PSTR(s) (s)


#include <stdio.h>
#include <stdlib.h>

// Прототип функции перевода float в строку, специфичный для AVR/Arduino, который требует Marlin
#ifdef __cplusplus
extern "C" {
#endif
  char* dtostrf(double __val, signed char __width, unsigned char __prec, char* __s);
#ifdef __cplusplus
}
#endif


// Объявление класса MarlinHAL для Marlin 3.0
class MarlinHAL {
public:
  static void init();
  static void idletask();

  static void reboot() { NVIC_SystemReset(); }
  static void clear_reset_source() {}
  static uint8_t get_reset_source() { return 0; }
};

extern MarlinHAL hal;

// Глобальные заглушки для функций управления GPIO
void pinMode(uint16_t pin, uint8_t mode);
void digitalWrite(uint16_t pin, uint8_t val);
bool digitalRead(uint16_t pin);

// Таймеры и время
typedef uint32_t hal_timer_t;
#define HAL_TIMER_TYPE_MAX 0xFFFFFFFF
typedef int8_t pin_t;

#define CRITICAL_SECTION_START() uint32_t primask = __get_PRIMASK(); __disable_irq()
#define CRITICAL_SECTION_END()   __set_PRIMASK(primask)

#define DELAY_CYCLES(C) do { \
  uint32_t cycles = (C); \
  __asm__ __volatile__ ( \
    "1: addi %0, %0, -1    \n\t" \
    "   bne  %0, zero, 1b  \n\t" \
    : "+r" (cycles) \
  ); \
} while(0)

unsigned long millis();
unsigned long micros();
void delay(const int ms);
#include "MarlinSerial.h"