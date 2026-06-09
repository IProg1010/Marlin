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

#include "fastio.h" // Подключаем пины в первую очередь!
#include "timers.h" // Подключаем пины в первую очередь!

#ifndef HAL_PLATFORM_CH32V307
  #define HAL_PLATFORM_CH32V307
#endif

#ifndef CPU_32_BIT
  #define CPU_32_BIT 1
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
#define INPUT_PULLDOWN 0x03 // Или любое свободное число, например 3

// Макросы прогмем (для AVR совместимости строк, на 32 битах они пустые)
#define PROGMEM
#define PSTR(s) (s)

#define analogInputToDigitalPin(p) (p)

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

typedef int8_t pin_t;
typedef uint8_t byte;

#define cli() __disable_irq()
#define sei() __enable_irq()

// --- НАСТРОЙКА АЦП (ADC) ДЛЯ CH32V307 ---
#define HAL_ADC_RESOLUTION  12  // 12-битный АЦП у QingKe V4F
typedef uint16_t raw_adc_t;      // Тип данных для хранения сырого значения АЦП (0..4095)

// Обязательные функции АЦП, которые Marlin будет вызывать для чтения термисторов
void HAL_adc_init();
void HAL_adc_start_conversion(const uint8_t ch);
raw_adc_t HAL_adc_get_result();

// Объявление класса MarlinHAL для Marlin 3.0
class MarlinHAL {
public:
  static void init();
  static void idletask();

  static void init_board(); 
  static uint32_t freeMemory(); 

  static void reboot() { NVIC_SystemReset(); }
  static void clear_reset_source() {}
  static uint8_t get_reset_source() { return 0; }

  static void watchdog_init() {}   
  static void watchdog_refresh() {} 

  static void set_pwm_duty(const pin_t pin, const uint16_t v) { /*UNUSED(pin); UNUSED(v);*/ }

  static void isr_off() { /*DISABLE_STEPPER_DRIVER_INTERRUPT();*/ }
  static void isr_on()  { ENABLE_STEPPER_DRIVER_INTERRUPT();  } // Добавляем эту строчку

  // Объектные методы АЦП для Marlin 3.0
  static void adc_init();
  static void adc_enable(const pin_t pin) { (void)pin; } // Заглушка активации канала
  static void adc_start(const uint8_t ch);
  static raw_adc_t adc_get_result();
  static uint8_t adc_ready() { return 0; }
  static raw_adc_t adc_value() { return 0; }
};

extern MarlinHAL hal;

// Глобальные заглушки для функций управления GPIO
void pinMode(uint16_t pin, uint8_t mode);
void digitalWrite(uint16_t pin, uint8_t val);
bool digitalRead(uint16_t pin);

// Таймеры и время
typedef uint32_t hal_timer_t;
#define HAL_TIMER_TYPE_MAX 0xFFFFFFFF

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


#ifdef __cplusplus
  // Используем шаблоны, чтобы не ломать std::min / std::max в STL заголовочниках
  template <class T, class L> inline auto min(const T a, const L b) -> decltype(a + b) { return (a < b) ? a : b; }
  template <class T, class L> inline auto max(const T a, const L b) -> decltype(a + b) { return (a > b) ? a : b; }
  
  template <class T, class L, class H>
  inline auto constrain(const T amt, const L low, const H high) -> decltype(amt + low + high) {
    return (amt < low) ? low : ((amt > high) ? high : amt);
  }
  
  template <typename T> inline T abs(const T x) { return (x > 0) ? x : -x; }

  #define MultiU32X24toH32(A, B) ((uint32_t)((uint32_t)(A) * (uint32_t)(B)))
#else
  // На случай, если файл подключит чистый C-партизан
  #ifndef min
    #define min(a,b) ((a)<(b)?(a):(b))
  #endif
  #ifndef max
    #define max(a,b) ((a)>(b)?(a):(b))
  #endif
  #ifndef constrain
    #define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
  #endif
  #ifndef abs
    #define abs(x) ((x)>0?(x):-(x))
  #endif
#endif


unsigned long millis();
unsigned long micros();
void delay(const int ms);
#include "MarlinSerial.h"