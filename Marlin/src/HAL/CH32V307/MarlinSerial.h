#pragma once
#include <stdint.h>
#include <stddef.h>
#include "ch32v30x.h"

class MarlinSerial {
public:
  void begin(const long baud);
  void end() {}
  
  void write(const uint8_t c);
  int read();
  int available();
  void flush() {}

  // Реализация методов print для совместимости с ядром Marlin 3.0
  void print(const char* str);
  void print(char c);
  void print(int n, int base = 10);
  void print(long n, int base = 10);
  void print(double n, int digits = 2);

  void println();
  void println(const char* str);
  void println(char c);
  void println(int n, int base = 10);
  void println(long n, int base = 10);
  void println(double n, int digits = 2);
};

// Объявляем глобальный объект нашего серийного порта
extern MarlinSerial customized_serial;

// Принудительно маппим макрос Marlin на наш кастомный объект порта
#ifndef MYSERIAL1
  #define MYSERIAL1 customized_serial
#endif