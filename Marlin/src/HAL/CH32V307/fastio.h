#pragma once
#include "ch32v30x.h"

// Упаковываем порт и пин в 16 бит: старший байт — порт (0=A, 1=B...), младший — номер пина (0..15)
#define NUM_TO_PIN(P, N)  ((uint16_t)(((P) << 8) | (N)))

#define PA0  NUM_TO_PIN(0, 0)
#define PA1  NUM_TO_PIN(0, 1)
#define PA2  NUM_TO_PIN(0, 2)
#define PA3  NUM_TO_PIN(0, 3)
#define PA4  NUM_TO_PIN(0, 4)
#define PA5  NUM_TO_PIN(0, 5)
#define PA6  NUM_TO_PIN(0, 6)
#define PA7  NUM_TO_PIN(0, 7)
#define PA9  NUM_TO_PIN(0, 9)
#define PA10 NUM_TO_PIN(0, 10)

#define PB0  NUM_TO_PIN(1, 0)
#define PB1  NUM_TO_PIN(1, 1)
#define PB10 NUM_TO_PIN(1, 10)
#define PB11 NUM_TO_PIN(1, 11)

#define PC0  NUM_TO_PIN(2, 0)
#define PC1  NUM_TO_PIN(2, 1)
#define PC6  NUM_TO_PIN(2, 6)
#define PC7  NUM_TO_PIN(2, 7)
#define PC8  NUM_TO_PIN(2, 8)
#define PC9  NUM_TO_PIN(2, 9)

#define PD0  NUM_TO_PIN(3, 0)
#define PD1  NUM_TO_PIN(3, 1)

// Обязательные базовые макросы Marlin для проверки валидности пинов
#define _READ(P)           0
#define _WRITE(P,V)        do{}while(0)
#define _TOGGLE(P)         do{}while(0)
#define _SET_INPUT(P)      do{}while(0)
#define _SET_OUTPUT(P)     do{}while(0)

#define pin_is_valid(P)    ((P) >= 0)