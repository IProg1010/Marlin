#pragma once

#include <stdint.h>

// Базовые макросы Marlin для работы с прерываниями таймеров
#define HAL_STEP_TIMER_NUM  2  // Будем использовать TIM2 для шагов
#define HAL_TEMP_TIMER_NUM  3  // Будем использовать TIM3 для температуры

#define FORCE_INLINE __attribute__((always_inline)) inline