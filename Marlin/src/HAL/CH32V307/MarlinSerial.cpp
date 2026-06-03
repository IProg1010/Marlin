#include "MarlinSerial.h"
#include "HAL.h"
#include <stdio.h>

MarlinSerial customized_serial;

void MarlinSerial::begin(const long baud) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    // Включаем тактирование GPIOA и USART1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // Настройка пина PA9 (USART1_TX) как Alternate Function Push-Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Настройка пина PA10 (USART1_RX) как Floating Input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Конфигурация параметров USART1
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void MarlinSerial::write(const uint8_t c) {
    // Ожидаем окончания передачи предыдущего байта (флаг TXE)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}

int MarlinSerial::read() {
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET) return -1;
    return USART_ReceiveData(USART1);
}

int MarlinSerial::available() {
    return (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) ? 1 : 0;
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
    char format[10];
    char buf[32];
    snprintf(format, sizeof(format), "%%.%df", digits);
    snprintf(buf, sizeof(buf), format, n);
    print(buf);
}

void MarlinSerial::println() { print("\r\n"); }
void MarlinSerial::println(const char* str) { print(str); println(); }
void MarlinSerial::println(char c)          { print(c); println(); }
void MarlinSerial::println(int n, int base) { print(n, base); println(); }
void MarlinSerial::println(long n, int base){ print(n, base); println(); }
void MarlinSerial::println(double n, int digits) { print(n, digits); println(); }