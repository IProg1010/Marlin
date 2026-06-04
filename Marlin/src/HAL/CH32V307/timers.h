#pragma once

#include <stdint.h>

/*// Базовые макросы Marlin для работы с прерываниями таймеров
#define HAL_STEP_TIMER_NUM  2  // Будем использовать TIM2 для шагов
#define HAL_TEMP_TIMER_NUM  3  // Будем использовать TIM3 для температуры
// Частота вызова прерывания таймера температуры (стандарт для Marlin)
#define TEMP_TIMER_FREQUENCY 1000U 

#define ENABLE_STEPPER_DRIVER_INTERRUPT()  NVIC_EnableIRQ(TIM2_IRQn)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() NVIC_DisableIRQ(TIM2_IRQn)
#define STEPPER_ISR_ENABLED()              (PFIC->ISR[0] & (1 << (TIM2_IRQn & 0x1F)))


#define FORCE_INLINE __attribute__((always_inline)) inline*/
#include "ch32v30x.h"

// Идентификаторы таймеров для Marlin 3.0
#define MF_TIMER_STEP  1
#define MF_TIMER_PULSE 1  // Пульс считает тот же TIM2
#define MF_TIMER_TEMP  2

// Аппаратные номера таймеров
#define HAL_STEP_TIMER_NUM  2
#define HAL_TEMP_TIMER_NUM  3

// Константы частот
#define STEPPER_TIMER_RATE   2000000UL // 2 МГц
#define TEMP_TIMER_FREQUENCY 1000U
#define STEPPER_TIMER_TICKS_PER_US ((STEPPER_TIMER_RATE) / 1000000UL)

// Сообщаем Marlin использовать честный 32-битный обсчет шагов без легаси AVR-таблиц
#define STEP_TIMER_SMALL_TICKS

// Векторы прерываний WCH SPL
#define HAL_STEP_TIMER_ISR() extern "C" __attribute__((interrupt("WCH-Interrupt-fast"))) void TIM2_IRQHandler()
#define HAL_TEMP_TIMER_ISR() extern "C" __attribute__((interrupt("WCH-Interrupt-fast"))) void TIM3_IRQHandler()

// Прологи прерываний
#define HAL_timer_isr_prologue(T) do { \
  if ((T) == MF_TIMER_STEP) TIM_ClearITPendingBit(TIM2, TIM_IT_Update); \
  else if ((T) == MF_TIMER_TEMP) TIM_ClearITPendingBit(TIM3, TIM_IT_Update); \
} while(0)
#define HAL_timer_isr_epilogue(T) do { } while(0)

// Контроль прерываний шагов (TIM2)
#define ENABLE_STEPPER_DRIVER_INTERRUPT()  NVIC_EnableIRQ(TIM2_IRQn)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() NVIC_DisableIRQ(TIM2_IRQn)
#define STEPPER_ISR_ENABLED()              (PFIC->ISR[0] & (1 << (TIM2_IRQn & 0x1F)))

// Контроль прерываний температуры (TIM3)
#define ENABLE_TEMPERATURE_INTERRUPT()     NVIC_EnableIRQ(TIM3_IRQn)
#define DISABLE_TEMPERATURE_INTERRUPT()    NVIC_DisableIRQ(TIM3_IRQn)

#define FORCE_INLINE __attribute__((always_inline)) inline

FORCE_INLINE static void HAL_timer_set_compare(const uint8_t timer_num, const uint32_t compare) {
  if (timer_num == MF_TIMER_STEP) TIM2->ATRLR = (uint16_t)(compare & 0xFFFF);
}

FORCE_INLINE static uint32_t HAL_timer_get_count(const uint8_t timer_num) {
  return (timer_num == MF_TIMER_STEP || timer_num == MF_TIMER_PULSE) ? TIM2->CNT : 0;
}

// Заглушка запуска таймера температуры
FORCE_INLINE static void HAL_timer_start(const uint8_t timer_num, const uint32_t freq) { (void)timer_num; (void)freq; }
