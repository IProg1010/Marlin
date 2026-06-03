/*#include "HAL.h"

volatile uint32_t system_millis = 0;

// Обработчик прерывания SysTick (аппаратное стекирование WCH Fast Interrupt)
extern "C" __attribute__((interrupt("WCH-Interrupt-fast"))) void SysTick_Handler(void) {
    system_millis++;
    SysTick->SR = 0; // Сброс флага прерывания SysTick
}

void HAL_init() {
    // Настраиваем SysTick на прерывания каждые 1 мс (1000 Гц)
    // Обновляем системную частоту (убеждаемся, что она 144МГц)
    SystemCoreClockUpdate();

    // Настройка системного таймера (SysTick) WCH на 1 мс
    // В SDK от WCH это делается через регистры CMP и CNT
    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = SystemCoreClock / 1000 - 1;
    SysTick->CTLR = 0xf; // Включаем таймер, прерывания и автоперезапуск

    NVIC_EnableIRQ(SysTick_IRQn);
}

unsigned long millis() {
    return system_millis;
}

unsigned long micros() {
    // Для MVP просто возвращаем приблизительное значение
    return system_millis * 1000;
}

void delay(const int ms) {
    uint32_t start = millis();
    while (millis() - start < (uint32_t)ms) { __NOP(); }
}

void idletask() {
    // Вызывается Marlin в пустых циклах ожидания
}*/
#include "HAL.h"


// Создаем экземпляр класса HAL для прошивки
MarlinHAL hal;

volatile uint32_t system_millis = 0;

extern "C" __attribute__((interrupt("WCH-Interrupt-fast"))) void SysTick_Handler(void) {
    system_millis++;
    SysTick->SR = 0;
}

// Привязываем функции к методам класса MarlinHAL
void MarlinHAL::init() {
    SystemCoreClockUpdate();
    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = SystemCoreClock / 1000 - 1;
    SysTick->CTLR = 0xf;
    NVIC_EnableIRQ(SysTick_IRQn);
}

void MarlinHAL::idletask() {
    // Вызывается в пустых циклах
}

// Обычные функции времени остаются глобальными
unsigned long millis() { return system_millis; }
unsigned long micros() { return system_millis * 1000; }
void delay(const int ms) {
    uint32_t start = millis();
    while (millis() - start < (uint32_t)ms) { __NOP(); }
}

extern "C" char* dtostrf(double __val, signed char __width, unsigned char __prec, char* __s) {
    // Формируем динамическую строку формата, например, "%10.4f"
    char format[16];
    snprintf(format, sizeof(format), "%%%d.%df", __width, __prec);
    
    // Записываем результат форматирования float напрямую в буфер прошивки
    snprintf(__s, __width > 0 ? __width + 1 : 32, format, __val);
    return __s;
}