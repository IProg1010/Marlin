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
#include "dev_eth_function.h"


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


netconfig net_config = {{192, 168, 3, 83}, {255, 255, 254, 0}, {192, 168, 3, 1}, {192, 168, 12, 83, 34, 67}};
void MarlinHAL::idletask() {
    // Вызывается в пустых циклах

    /*static uint32_t toggle_cnt = 0;
    if (++toggle_cnt >= 100000) { 
        TOGGLE(PC0); 
        toggle_cnt = 0;
    }*/
    lwip_loop();
}

MarlinSPI customized_spi3(PC12, PC11, PC10, PB6); 

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
    
    set_net_config(&net_config);

    lwip_initialize();
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

extern "C" char* dtostrf(double val, signed char width, unsigned char prec, char* sout) {
    char fmt[16]; // Выделяем честный массив на 16 байт
    snprintf(fmt, sizeof(fmt), "%%%d.%df", width, prec);
    sprintf(sout, fmt, val);
    return sout;
}
// Инициализируем статическую переменную класса (не забудьте объявить её в заголовочнике)
uint16_t MarlinHAL::adc_result = 0;

// 1. Вызывается один раз при старте прошивки
void MarlinHAL::adc_init() {
    ADC_InitTypeDef ADC_InitStructure = {0};

    // Включаем тактирование модуля ADC1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    // Частота АЦП: 144 МГц / 8 = 8 МГц (в пределах допустимых 14 МГц)
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    // Базовая конфигурация ADC1
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 12 бит, правое выравнивание
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // Включаем АЦП
    ADC_Cmd(ADC1, ENABLE);

    // Калибровка (Обязательна для CH32V307!)
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

// 2. Настройка конкретной ножки под АЦП
void MarlinHAL::adc_enable(const uint8_t pin) {
    // Используем вашу готовую и проверенную функцию pinMode!
    // В Marlin режим INPUT_ANALOG на уровне вашей pinMode должен переводить пин в GPIO_Mode_AIN
    pinMode(pin, INPUT_ANALOG); 
}

// 3. Запуск конверсии на пине (Вызывается из Temperature::isr)
void MarlinHAL::adc_start(const uint8_t pin) {
    //printf("adc start\r\n");
    // Переводим пин Marlin во внутренний номер канала АЦП CH32
    // Напишите тут вашу функцию маппинга, если пины плат не совпадают с каналами АЦП.
    // Если TEMP_0_PIN в pins_*.h равен 0 (для PA0), то канал = 0.
    uint8_t adc_channel = (uint8_t)pin; 
    if (adc_channel > 15) return;

    // Конфигурируем выбранный канал термистора
    ADC_RegularChannelConfig(ADC1, adc_channel, 1, ADC_SampleTime_239Cycles5);

    // Запускаем преобразование аппаратно
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // НАДЕЖНЫЙ СИНХРОННЫЙ ПУТЬ:
    // Ждем окончания конверсии прямо здесь, чтобы гарантировать ядру Marlin наличие данных
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

    // Сохраняем результат в статическую переменную класса, которую ждет метод adc_value()
    adc_result = ADC_GetConversionValue(ADC1);
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

extern "C" {

    void delay_ms(uint32_t ms) {
        // Используем твою уже готовую и проверенную функцию delay() из HAL Marlin
        delay(ms); 
    }

    void delay_us(uint32_t us) {
        // Микросекундная задержка на базе аппаратного счетчика SysTick чипа CH32V307
        // Частота 144 МГц означает, что в 1 микросекунде ровно 144 тика процессора
        uint32_t start_ticks = SysTick->CNT;
        uint32_t ticks_to_wait = us * (SystemCoreClock / 1000000UL);
        
        while ((SysTick->CNT - start_ticks) < ticks_to_wait) {
            __NOP(); // Крутимся в цикле, пока не пройдет нужное количество микросекунд
        }
    }

}