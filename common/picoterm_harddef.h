#ifndef _PICOTERM_HARDDEF_H_
#define _PICOTERM_HARDDEF_H_


/* ==========================================================================
                   Hardware definition for Picoterm
   ========================================================================== */

#define LED             25
#define UART_ID         uart1   // also see hid_app.c
#define UART_TX_PIN     20
#define UART_RX_PIN     21

#define USB_POWER_GPIO 26 // this GPIO can be used with a MOSFET to power-up USB
#define USB_POWER_DELAY 5000 // ms

#define BUZZER_GPIO 27 // active buzzer


// Hardware Rev 1.0 - Original Picoterm based on the Raspberry Pi Pico
/*
#define SDA_PIN 26
#define SCL_PIN 27
#define DEBUG_TX  28
#define DEBUG_BAUD  115200
#define SPI_SD_SCK_PIN NULL
#define SPI_SD_TX_PIN  NULL
#define SPI_SD_RX_PIN  NULL
#define SPI_SD_CSN_PIN NULL
*/

// Hardware Rev 1.1 - RP2040 Picoterm based on RP2040 chip
#define SDA_PIN 18
#define SCL_PIN 19
#define DEBUG_TX  22
#define DEBUG_BAUD  115200
#define SPI_SD_SCK_PIN 26
#define SPI_SD_TX_PIN  27
#define SPI_SD_RX_PIN  28
#define SPI_SD_CSN_PIN 5

#endif // _PICOTERM_HARDDEF_H_
