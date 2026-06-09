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
#include "MarlinSerial.h"
// Идентификаторы таймеров для Marlin 3.0
#define MF_TIMER_STEP  2  // Шаги на TIM2
#define MF_TIMER_PULSE 2  // Пульс на TIM2
#define MF_TIMER_TEMP  3  // Температура на TIM3

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
  if ((T) == 2) TIM_ClearITPendingBit(TIM2, TIM_IT_Update); \
  else if ((T) == 3) TIM_ClearITPendingBit(TIM3, TIM_IT_Update); \
} while(0)

#define HAL_timer_isr_epilogue(T) do { } while(0)

// Контроль прерываний шагов (TIM2)
#define ENABLE_STEPPER_DRIVER_INTERRUPT()  do { \
  TIM_Cmd(TIM2, ENABLE);   \
  NVIC_EnableIRQ(TIM2_IRQn); \
} while(0)

#define DISABLE_STEPPER_DRIVER_INTERRUPT() do { \
  NVIC_DisableIRQ(TIM2_IRQn); \
  TIM_Cmd(TIM2, DISABLE);    \
} while(0)
#define STEPPER_ISR_ENABLED()              ((!!(PFIC->IENR[0] & (1 << (TIM2_IRQn & 0x1F)))))

// Контроль прерываний температуры (TIM3)
#define ENABLE_TEMPERATURE_INTERRUPT()     do { \
  TIM_Cmd(TIM3, ENABLE);   \
  NVIC_EnableIRQ(TIM3_IRQn); \
} while(0)

#define DISABLE_TEMPERATURE_INTERRUPT()    do { \
  NVIC_DisableIRQ(TIM3_IRQn); \
  TIM_Cmd(TIM3, DISABLE);    \
} while(0)


#define FORCE_INLINE __attribute__((always_inline)) inline

FORCE_INLINE static void HAL_timer_set_compare(const uint8_t timer_num, const uint32_t compare) {
  if (timer_num == MF_TIMER_STEP) {
    TIM2->ATRLR = (uint16_t)(compare & 0xFFFF);
  }
  else if (timer_num == MF_TIMER_TEMP) {
    TIM3->ATRLR = (uint16_t)(compare & 0xFFFF);
  }
}

FORCE_INLINE static uint32_t HAL_timer_get_count(const uint8_t timer_num) {
  if (timer_num == MF_TIMER_STEP) return TIM2->CNT;
  if (timer_num == MF_TIMER_TEMP) return TIM3->CNT;
  return 0;
}

// Заглушка запуска таймера температуры
FORCE_INLINE static void HAL_timer_start(const uint8_t timer_num, const uint32_t freq) { 
    customized_serial.print("-> HAL_timer_start for T=");
    customized_serial.println((int)timer_num);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    if (timer_num == MF_TIMER_STEP) {
      // --- НАСТРОЙКА TIM2 (ШАГОВИКИ) ---
      
    customized_serial.println("  Configuring TIM2...");
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

      // Вычисляем делитель (Prescaler), чтобы таймер тикал строго на частоте STEPPER_TIMER_RATE (2 МГц)
      // Системная частота шины APB1 для TIM2 при 144 МГц процессора обычно равна 144 МГц (или 72 МГц с умножением на 2)
      uint16_t prescaler = (SystemCoreClock / STEPPER_TIMER_RATE) - 1;

      TIM_TimeBaseStructure.TIM_Period = (uint16_t)((STEPPER_TIMER_RATE / freq) - 1);
      TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
      TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
      TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
      TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

      // Разрешаем прерывание по обновлению
      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

      // Настраиваем PFIC (NVIC) для TIM2
      NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // САМЫЙ ВЫСОКИЙ ПРИОРИТЕТ ДЛЯ ШАГОВ!
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);

      // Включаем таймер
      //TIM_Cmd(TIM2, ENABLE);
    customized_serial.println("  Configuring TIM2... ok");
    } 
    else if (timer_num == MF_TIMER_TEMP) {

    customized_serial.println("  Configuring TIM3...");
      // --- НАСТРОЙКА TIM3 (ТЕМПЕРАТУРА) ---
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

      // Настраиваем TIM3 так, чтобы он выдавал ровно freq (обычно 1000 Гц = 1 мс)
      // Будем тактировать его делителем покороче, например, чтобы он работал на 1 МГц
      uint16_t prescaler = (SystemCoreClock / 1000000UL) - 1;

      TIM_TimeBaseStructure.TIM_Period = (uint16_t)((1000000UL / freq) - 1);
      TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
      TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
      TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
      TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

      TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

      // Настраиваем PFIC (NVIC) для TIM3
      NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // Приоритет пониже, чем у шагов
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);

    customized_serial.println("  Configuring TIM3... ok");
      // Включаем таймер
      //TIM_Cmd(TIM3, ENABLE);
    }                                     
}
