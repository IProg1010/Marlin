#include "MarlinSerial.h"
#include "HAL.h"
#include <stdio.h>

MarlinSerial customized_serial;
#define RX_BUFFER_SIZE 256 // Размер буфера (желательно кратный 2)

// Буфер, куда DMA будет складывать данные напрямую
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
// Указатель чтения для Marlin (хвост очереди)
static volatile uint16_t rx_tail = 0;

void MarlinSerial::begin(const long baud) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    DMA_InitTypeDef DMA_InitStructure = {0};

    // 1. Включаем тактирование GPIOA, USART1 и DMA1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // 2. Настройка пинов PA9 (TX) и PA10 (RX)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. Конфигурация USART1
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    // 4. Настройка DMA1 Channel 5 для приема (USART1_RX)
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DATAR); // Адрес регистра данных USART
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx_buffer;          // Наш буфер в SRAM
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                    // Направление: из периферии в память
    DMA_InitStructure.DMA_BufferSize = RX_BUFFER_SIZE;                    // Размер кольца
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;       // Адрес USART не инкрементируем
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;               // Адрес памяти инкрементируем
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;// Читаем по 1 байту
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;       // Пишем по 1 байту
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                       // КРИТИЧНО: Режим кольцевого буфера
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                   // Высокий приоритет
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                          // Не memory-to-memory
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

    // Включаем канал DMA
    DMA_Cmd(DMA1_Channel5, ENABLE);

    // 5. Даем команду USART отправлять запросы в DMA при получении байта
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    // Сбрасываем программный хвост буфера
    rx_tail = 0;

    // Включаем USART
    USART_Cmd(USART1, ENABLE);

    for (const char *p = "\r\nHellow RISC-V Marlin!\r\n"; *p; p++) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *p);
    }

      this->println("\r\n=== CORE FLOAT TEST START ===");
    
    double test_value = 123.456;
    this->print("Testing customized print(double): ");
    this->println(test_value, 3); // Должно напечатать 123.456

    // Проверим сырой snprintf, который используется у вас внутри класса
    char test_buf[32];
    snprintf(test_buf, sizeof(test_buf), "%f", 98.76);
    this->print("Testing raw snprintf(%%f): ");
    this->println(test_buf); // Должно напечатать 98.760000

    this->println("=== CORE FLOAT TEST END ===\r\n");
}

// Функция возвращает текущую позицию, куда DMA пишет данные прямо сейчас
inline uint16_t get_dma_head() {
    // Регистр CNDTR считает НАЗАД от RX_BUFFER_SIZE до 0. 
    // Переводим его в классический индекс массива (от 0 до RX_BUFFER_SIZE-1)
    return (RX_BUFFER_SIZE - DMA1_Channel5->CNTR) % RX_BUFFER_SIZE;
}

void MarlinSerial::write(const uint8_t c) {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}

void MarlinSerial::flushTX() {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

int MarlinSerial::available() {
    uint16_t head = get_dma_head(); 
    return (uint16_t)(RX_BUFFER_SIZE + head - rx_tail) % RX_BUFFER_SIZE;
}

int MarlinSerial::read() {
    uint16_t head = get_dma_head();
    if (head == rx_tail) return -1; // Новых данных нет (буфер пуст)

    uint8_t c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE; // Двигаем хвост вперед
    return c;
}

// Базовые функции текстового вывода через write()
void MarlinSerial::print(const char* str) { while (*str) write(*str++); }
void MarlinSerial::print(char c)          { write(c); }

void MarlinSerial::print(int n, int base) {
    char buf[12];
    snprintf(buf, sizeof(buf), (base == 16) ? "%x" : "%d", n);
    print(buf);
}

void MarlinSerial::print(long n, int base) {
    char buf[12];
    snprintf(buf, sizeof(buf), (base == 16) ? "%lx" : "%ld", n);
    print(buf);
}

void MarlinSerial::print(double n, int digits) {
        // Обработка отрицательных чисел
    if (n < 0.0) {
        print('-');
        n = -n;
    }

    // Округление в зависимости от нужной точности (digits)
    double rounding = 0.5;
    for (int i = 0; i < digits; ++i) {
        rounding /= 10.0;
    }
    n += rounding;

    // Выделяем целую часть
    long integer_part = (long)n;
    print(integer_part, 10); // Выводим целое число через ваш рабочий метод

    // Если нужны знаки после запятой
    if (digits > 0) {
        print('.'); // Ставим точку
        
        // Выделяем дробную часть
        double remainder = n - (double)integer_part;
        long fractional_part = 0;
        
        // Превращаем дробь в целое число (например, 0.5 -> 5)
        for (int i = 0; i < digits; ++i) {
            remainder *= 10.0;
        }
        fractional_part = (long)remainder;

        // Важно: если дробная часть имеет ведущие нули (например, 25.05),
        // нужно вывести этот ноль перед цифрой 5.
        char format[12];
        snprintf(format, sizeof(format), "%%0%dd", digits); // Собирает формат типа "%01d" или "%02d"
        
        char buf[16];
        snprintf(buf, sizeof(buf), format, fractional_part);
        print(buf);
    }
}

void MarlinSerial::print(unsigned int n, int base) {
    char buf[12];
    snprintf(buf, sizeof(buf), (base == 16) ? "%x" : "%u", n);
    print(buf);
}

void MarlinSerial::print(unsigned long n, int base) {
    char buf[12];
    snprintf(buf, sizeof(buf), (base == 16) ? "%lx" : "%lu", n);
    print(buf);
}

void MarlinSerial::println() { print("\r\n"); }
void MarlinSerial::println(const char* str) { print(str); println(); }
void MarlinSerial::println(char c)          { print(c); println(); }
void MarlinSerial::println(int n, int base) { print(n, base); println(); }
void MarlinSerial::println(long n, int base){ print(n, base); println(); }
void MarlinSerial::println(double n, int digits) { print(n, digits); println(); }
void MarlinSerial::println(unsigned int n, int base) { print(n, base); println(); }
void MarlinSerial::println(unsigned long n, int base){ print(n, base); println(); }

// Сигнатура чистого Си для связи с ассемблерным startup-файлом
extern "C" {
    void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
}

void HardFault_Handler(void) {
    // Отправляем символ '!' напрямую в регистр данных USART1 на полной скорости
    while ((USART1->STATR & USART_FLAG_TXE) == 0);
    USART1->DATAR = '!';
    
    // Бесконечно висим
    while (1) {
        __asm__ volatile("nop");
    }
}

