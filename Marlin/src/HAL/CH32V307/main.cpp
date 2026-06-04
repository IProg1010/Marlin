#include "HAL.h"

// Объявляем внешние функции Marlin
extern void setup();
extern void loop();

int main(void) {
    // 1. Вызываем инициализацию нашего HAL
    hal.init();

    // 2. Запускаем стандартную инициализацию Marlin
    setup();

    // 3. Бесконечный цикл планировщика Marlin
    for (;;) {
        loop();
    }
    
    return 0;
}
