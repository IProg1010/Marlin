
#include "MarlinSPI.h"
#include "ch32v30x_spi.h"
#include "ch32v30x_gpio.h"
#include "MarlinSerial.h"

MarlinSPI::MarlinSPI(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t ssel) 
  : _mosiPin(mosi), _misoPin(miso), _sckPin(sclk), _ssPin(ssel) {
    
    _bitOrder = 1; // MSBFIRST
    _dataMode = 0; // SPI_MODE_0
    _clockDivider = SPI_MIN_SPEED; // Начинаем с минимальной скорости
    _mustInit = true;

    // Автоматически определяем аппаратный модуль CH32 по пину тактирования SCK
    // Предположим, в твоем маппинге пинов: PB3 — это SPI3, PB13 — SPI2, PA5 — SPI1.
    // Подставь логику твоих индексов пинов:
    if (_sckPin == 3) {       // Если SCK привязан к PB3
        _spiInstance = SPI3;
    } else if (_sckPin == 13) { // Если к PB13
        _spiInstance = SPI2;
    } else {
        _spiInstance = SPI1;
    }
}

void MarlinSPI::begin(void) {
    if (_mustInit) {
        initHardware();
        _mustInit = false;
    }
}

void MarlinSPI::initHardware() {

    customized_serial.println("\r\n[SPI3_DBG] Entering initHardware()...");
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef SPI_InitStructure = {0};

    if (_spiInstance == SPI3) {
        // --- ИНИЦИАЛИЗАЦИЯ SPI3 ---
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

        // Освобождаем пины от JTAG
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        // SCK (PB3) и MOSI (PB5) -> Alternate Function Push-Pull
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        // MISO (PB4) -> Input Floating
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOB, &GPIO_InitStructure);
    }
    // Здесь можно дописать блоки "else if (_spiInstance == SPI2)" при необходимости...

    // Настройка параметров самого SPI модуля CH32
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    
    // Применяем выбранный режим данных Mode 0..3
    if (_dataMode == 3) { // SPI_MODE_3 (Часто для SD)
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    } else {              // По умолчанию Mode 0
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    }

    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    
    // Устанавливаем делитель скорости
    setClockDivider(_clockDivider);
    SPI_InitStructure.SPI_BaudRatePrescaler = _spiInstance->CTLR1 & (7 << 3); 

    SPI_InitStructure.SPI_FirstBit = (_bitOrder == 1) ? SPI_FirstBit_MSB : SPI_FirstBit_LSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(_spiInstance, &SPI_InitStructure);

    SPI_Cmd(_spiInstance, ENABLE);
    customized_serial.println("\r\n[SPI3_DBG] Entering initHardware() OK\r\n");
}

uint8_t MarlinSPI::transfer(uint8_t data) {
    uint32_t timeout;

    // 1. Защита буфера передачи
    timeout = 100000;
    while (SPI_I2S_GetFlagStatus(_spiInstance, SPI_I2S_FLAG_TXE) == RESET) {
        if (--timeout == 0) return 0xFF; // Выходим по таймауту, если шина лежит
    }
    SPI_I2S_SendData(_spiInstance, data);

    // 2. Защита буфера приема
    timeout = 100000;
    while (SPI_I2S_GetFlagStatus(_spiInstance, SPI_I2S_FLAG_RXNE) == RESET) {
        if (--timeout == 0) return 0xFF; // Выходим по таймауту, если карта не ответила
    }
    
    return SPI_I2S_ReceiveData(_spiInstance);
}

// Пакетный обмен блоками (Marlin вызывает для секторов SD-карты по 512 байт)
void MarlinSPI::transfer(const uint8_t *block, uint8_t *data, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        uint8_t tx = block ? block[i] : 0xFF;
        uint8_t rx = transfer(tx);
        if (data) data[i] = rx;
    }
}

void MarlinSPI::setClockDivider(uint8_t div) {
    _clockDivider = div;
    uint16_t pr_mask = SPI_BaudRatePrescaler_256; // По умолчанию самая низкая скорость

    if (_spiInstance == SPI3) {
        // Так как SPI3 сидит на шине APB1 (72 МГц максимум):
        switch (div) {
            case SPI_FULL_SPEED:    pr_mask = SPI_BaudRatePrescaler_2;   // 72/2 = 36 МГц
                                    break;
            case SPI_HALF_SPEED:    pr_mask = SPI_BaudRatePrescaler_4;   // 72/4 = 18 МГц
                                    break;
            case SPI_QUARTER_SPEED: pr_mask = SPI_BaudRatePrescaler_8;   // 72/8 = 9 МГц
                                    break;
            case SPI_MIN_SPEED:     pr_mask = SPI_BaudRatePrescaler_256; // 72/256 = ~280 кГц (безопасно для старта)
                                    break;
        }
    }
    
    // Меняем делитель в регистре на лету
    _spiInstance->CTLR1 = (_spiInstance->CTLR1 & ~(7 << 3)) | pr_mask;
}