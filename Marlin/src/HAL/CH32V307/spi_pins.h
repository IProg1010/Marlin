#pragma once

// Настройка аппаратного SPI1 для CH32V307
#ifndef SD_SCK_PIN
  #define SD_SCK_PIN   PA5
#endif
#ifndef SD_MISO_PIN
  #define SD_MISO_PIN  PA6
#endif
#ifndef SD_MOSI_PIN
  #define SD_MOSI_PIN  PA7
#endif
#ifndef SD_SS_PIN
  #define SD_SS_PIN    PA4
#endif