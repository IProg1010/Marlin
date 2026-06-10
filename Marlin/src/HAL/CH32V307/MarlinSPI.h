#pragma once

//#include "HAL.h"
#include <stdint.h>
#include "ch32v30x.h"
#include <ctype.h>

// Константы для совместимости с Marlin
#define SPI_FULL_SPEED    0 // Максимальная скорость для работы с файлами
#define SPI_HALF_SPEED    1 // Средняя
#define SPI_QUARTER_SPEED 2 // Пониженная
#define SPI_MIN_SPEED     3 // Минимальная для инициализации карт (около 400 кГц)

#define SPI_SPI_SPEED_CLOCK_DIV2_MHZ  SPI_FULL_SPEED
#define SPI_SPI_SPEED_CLOCK_DIV4_MHZ  SPI_HALF_SPEED
#define SPI_SPI_SPEED_CLOCK_DIV8_MHZ  SPI_QUARTER_SPEED
#define SPI_SPI_SPEED_CLOCK_DIV256_MHZ SPI_MIN_SPEED

// Алиасы режимов данных, используемые в Marlin
#define SPI_MODE0 0
#define SPI_MODE3 3

class MarlinSPI {
public:
  // Конструктор принимает пины Marlin. Мы определим по ним, какой аппаратный SPI запустить
  MarlinSPI(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t ssel);

  void begin(void);
  void end(void) {}

  // Главный метод, который Marlin вызывает миллионы раз
  uint8_t transfer(uint8_t data);
  
  // Метод для пакетной передачи данных (Marlin использует его для блочного чтения SD)
  void transfer(const uint8_t *block, uint8_t *data, uint16_t count);

  // Методы настройки параметров
  void setBitOrder(uint8_t order) { _bitOrder = order; _mustInit = true; }
  void setDataMode(uint8_t mode)   { _dataMode = mode;  _mustInit = true; }
  void setClockDivider(uint8_t div);

private:
  void initHardware(); // Аппаратная инициализация выбранного SPI

  SPI_TypeDef* _spiInstance; // Указатель на структуру SPI1, SPI2 или SPI3
  uint8_t _mosiPin;
  uint8_t _misoPin;
  uint8_t _sckPin;
  uint8_t _ssPin;

  uint8_t _bitOrder;
  uint8_t _dataMode;
  uint8_t _clockDivider;
  bool _mustInit;
};
// 2. Исправленные и дополненные макросы для Sd2Card.cpp
#define spiInit(rate)          customized_spi3.setClockDivider(rate)
#define spiBegin()             customized_spi3.begin()
#define spiSend(data)          customized_spi3.transfer(data)
#define spiRec()               customized_spi3.transfer(0xFF)

// Чтение пачки байт (Marlin использует его в readData)
#define spiRead(buf, count)    customized_spi3.transfer(nullptr, (uint8_t*)(buf), count)

// Полностью безопасные пакетные макросы (без do-while конструкции, ломающей синтаксис)
#define spiReceiveBlock(buf, count) customized_spi3.transfer(nullptr, (uint8_t*)(buf), count)
#define spiSendBlock(token, buf) do { \
  customized_spi3.transfer((uint8_t)(token)); \
  customized_spi3.transfer((const uint8_t*)(buf), nullptr, 512); \
} while(0)
