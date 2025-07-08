#ifndef _FLASH_FLASH_H
#define _FLASH_FLASH_H


#include <stdint.h>

// Configuration: GPIOs for the SPI interface
#define SIM_FLASH_SPI_MOSI 2
#define SIM_FLASH_SPI_SCK  3  // Must be MOSI + 1
#define SIM_FLASH_SPI_CS   4  // Must be MOSI + 2
#define SIM_FLASH_SPI_MISO 5

// The PIO SMs and DMA channels are hardcoded as using dynamic
// allocation and then reading the values from memory is slightly slower.

// Configuration: PIO and SMs to use
// A different PIO must be used for each of read and write 
// as the total program size is too large to fit in one PIO
#define SIM_FLASH_pio_read_sm   1
#define SIM_FLASH_pio_read      pio1
#define SIM_FLASH_pio_write_sm  1
#define SIM_FLASH_pio_write     pio0

// Configuration: DMA channels
#define SIM_FLASH_rx_channel    0
#define SIM_FLASH_tx_channel    1
#define SIM_FLASH_tx_channel2   2
#define SIM_FLASH_tx_channel3   4


typedef enum {
    RECEIVED_CMD = 0,
    UNKNOWN_CMD = 0xff
}MSG_TYPE;

typedef struct element {
    int value;
    char message[0x100];
} element_t;
extern queue_t sample_fifo;


// Setup the simulated flash and launch core1 to service the commands.
//
// It is best to call this before other initialization, so that
// the hardcoded DMA channels and SMs are claimed before other resources
// are claimed.
void setup_simulated_flash();

#endif