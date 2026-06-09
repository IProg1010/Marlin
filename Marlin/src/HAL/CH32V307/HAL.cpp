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
//MarlinHAL hal;

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

// Объявляем внешние маркеры линкера для расчета свободной памяти
extern "C" char _end;
extern "C" char _heap_start;

void MarlinHAL::init_board() {
    // Включаем тактирование всех основных портов GPIO, чтобы пины принтера были готовы к работе
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | 
                           RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);
                           
    // Настраиваем отладочный светодиод на плате (PA0), если он используется
    #ifdef LED_PIN
      pinMode(LED_PIN, OUTPUT);
      digitalWrite(LED_PIN, LOW);
    #endif
}

uint32_t MarlinHAL::freeMemory() {
    // Получаем текущее значение указателя стека (Stack Pointer) в RISC-V QingKe V4F
    register char* stack_ptr __asm__("sp");
    
    // Свободная память — это расстояние между вершиной стека и концом кучи/статических данных
    return (uint32_t)(stack_ptr - &_end);
}


// Обычные функции времени остаются глобальными
unsigned long millis() { return system_millis; }
unsigned long micros() {     
    uint32_t ticks = SysTick->CNT; // Текущие тики внутри текущей миллисекунды
    uint32_t ms = system_millis;
    
    // Переводим тики процессора в микросекунды.
    // Частота 144 МГц означает, что 144 тика = 1 микросекунда.
    return (ms * 1000) + (ticks / (SystemCoreClock / 1000000)); 
}

void delay(const int ms) {
    uint32_t start = millis();
    while (millis() - start < (uint32_t)ms) { __NOP(); }
}

extern "C" char* dtostrf(double __val, signed char __width, unsigned char __prec, char* __s) {
    // 1. Обработка знака
    bool negative = false;
    if (__val < 0.0) {
        negative = true;
        __val = -__val;
    }

    // 2. Округление до заданной точности
    double rounding = 0.5;
    for (int i = 0; i < __prec; ++i) rounding /= 10.0;
    __val += rounding;

    // 3. Выделение целой части
    long int_part = (long)__val;

    // 4. Выделение дробной части
    double diff = __val - (double)int_part;
    long frac_part = 1;
    for (int i = 0; i < __prec; i++) frac_part *= 10;
    long frac_val = (long)(diff * frac_part);

    if (frac_val >= frac_part) {
        int_part++;
        frac_val -= frac_part;
    }

    // 5. Безопасная сборка строки через целые числа (которые работают всегда)
    char temp_buf[32];
    if (__prec > 0) {
        // Форматируем дробную часть с ведущими нулями (например, %01ld или %02ld)
        char frac_fmt[10];
        snprintf(frac_fmt, sizeof(frac_fmt), "%%ld.%%0%dld", __prec);
        snprintf(temp_buf, sizeof(temp_buf), frac_fmt, int_part, frac_val);
    } else {
        snprintf(temp_buf, sizeof(temp_buf), "%ld", int_part);
    }

    // 6. Добавляем минус, если число было отрицательным
    if (negative) {
        snprintf(__s, __width > 0 ? __width + 1 : 32, "-%s", temp_buf);
    } else {
        snprintf(__s, __width > 0 ? __width + 1 : 32, "%s", temp_buf);
    }

    return __s;
}
void MarlinHAL::adc_init() {
    ADC_InitTypeDef ADC_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

// Было: void HAL_adc_start_conversion(const uint8_t ch)
// Стало:
void MarlinHAL::adc_start(const uint8_t ch) {
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// Было: raw_adc_t HAL_adc_get_result()
// Стало:
raw_adc_t MarlinHAL::adc_get_result() {
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}


// Реализация pinMode с использованием типов uint16_t и uint8_t строго как в HAL.h
void pinMode(uint16_t pin, uint8_t mode) {
    customized_serial.print("[pin:");
    customized_serial.print((int)pin);
    customized_serial.print("]");

    if (pin == 9 || pin == 10) return;
    // Жесткая проверка валидности пина (учитываем, что 65535 — это бывший -1)
    if (pin == 0xFFFF || !pin_is_valid(pin)) return;

    GPIO_TypeDef* port = PIN_TO_PORT(pin);
    if (!port) return; // Защита от нулевого указателя порта

    uint16_t bitmask = PIN_TO_BITMASK(pin);
    if (bitmask == 0) return; // Защита от пустой маски

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = bitmask;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if (mode == OUTPUT) {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      // Push-Pull выход
    } else if (mode == INPUT_PULLUP) {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         // Вход с подтяжкой к VCC
    } else if (mode == INPUT_PULLDOWN) {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;         // Вход с подтяжкой к GND
    } else {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Обычный вход
    }

    GPIO_Init(port, &GPIO_InitStructure);
}

// Реализация digitalWrite
void digitalWrite(uint16_t pin, uint8_t val) {
    if (!pin_is_valid(pin)) return;
    
    if (val) {
        PIN_TO_PORT(pin)->BSHR = PIN_TO_BITMASK(pin); // Установить бит атомарно
    } else {
        PIN_TO_PORT(pin)->BCR = PIN_TO_BITMASK(pin);  // Сбросить бит атомарно
    }
}

// Реализация digitalRead
bool digitalRead(uint16_t pin) {
    if (!pin_is_valid(pin)) return false;
    return (PIN_TO_PORT(pin)->INDR & PIN_TO_BITMASK(pin)) ? true : false;
}
