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
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
                           
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
    if (!pin_is_valid(pin)) return;

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    // Автоматически включаем тактирование нужного порта при конфигурации пина
    GPIO_TypeDef* port = PIN_TO_PORT(pin);
    uint32_t rcc_periph = 0;
    if (port == GPIOA) rcc_periph = RCC_APB2Periph_GPIOA;
    else if (port == GPIOB) rcc_periph = RCC_APB2Periph_GPIOB;
    else if (port == GPIOC) rcc_periph = RCC_APB2Periph_GPIOC;
    else if (port == GPIOD) rcc_periph = RCC_APB2Periph_GPIOD;
    else if (port == GPIOE) rcc_periph = RCC_APB2Periph_GPIOE;
    
    if (rcc_periph) RCC_APB2PeriphClockCmd(rcc_periph, ENABLE);

    GPIO_InitStructure.GPIO_Pin = PIN_TO_BITMASK(pin);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if (mode == OUTPUT) {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // Push-Pull выход
    } else if (mode == INPUT_PULLUP) {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    // Вход с подтяжкой к VCC
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
