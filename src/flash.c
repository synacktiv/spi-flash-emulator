#include <hardware/pio.h>
#include <hardware/dma.h>
#include <pico/multicore.h>
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/ssi.h"
#include "pico/util/queue.h"
#include <stdio.h>
#include <hardware/clocks.h>

#include "flash.pio.h"
#include "flash.h"

#define RAM_BUF_SIZE 65536

queue_t sample_fifo;
const int FIFO_LENGTH = 0x200;

#define pio_write_offset 0

static int pio_read_offset;

uint8_t __attribute__((section(".spi_ram.emu_ram"))) emu_ram[RAM_BUF_SIZE];


static void setup_flash_pio()
{
    pio_read_offset = pio_add_program(SIM_FLASH_pio_read, &flash_read_program);
    pio_sm_claim(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm);
    pio_add_program_at_offset(SIM_FLASH_pio_write, &flash_write_program, pio_write_offset);
    pio_sm_claim(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm);

    flash_read_program_init(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm, pio_read_offset, SIM_FLASH_SPI_MOSI);
    flash_write_program_init(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm, pio_write_offset, SIM_FLASH_SPI_MOSI, SIM_FLASH_SPI_MISO);
}

static void setup_rx_channel()
{
    /* This DMA channel is using in case of writing (Not implemented yet) */
    dma_channel_claim(SIM_FLASH_rx_channel);
}

static void setup_tx_channel()
{
    dma_channel_config c;

    /* This DMA channel is used to transfer data from RAM buffer to PIO TX FIFO */
    dma_channel_claim(SIM_FLASH_tx_channel);    
    c = dma_channel_get_default_config(SIM_FLASH_tx_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_bswap(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm, true));
    dma_channel_configure(
        SIM_FLASH_tx_channel,          
        &c,            
        &SIM_FLASH_pio_write->txf[SIM_FLASH_pio_write_sm],           // The initial write address
        emu_ram,           // The initial read address
        10,             // Number of transfers
        false           // Start immediately.
    );

    /* This DMA channel is used to transfer data from SSI to RAM buffer */
    dma_channel_claim(SIM_FLASH_tx_channel3);
    c = dma_channel_get_default_config(SIM_FLASH_tx_channel3);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_bswap(&c, true);
    channel_config_set_ring(&c, true, 4);
    channel_config_set_dreq(&c, DREQ_XIP_SSIRX);
    channel_config_set_chain_to(&c, SIM_FLASH_tx_channel);
    dma_channel_configure(
        SIM_FLASH_tx_channel3,          
        &c,             
        &emu_ram,           // The initial write address
        &ssi_hw->dr0,           // The initial read address
        4,              // Number of transfers
        false           // Start immediately.
    );

    /* This DMA channel is used to autoconfigure SIM_FLASH_tx_channel SRC buffer */
    dma_channel_claim(SIM_FLASH_tx_channel2);
    c = dma_channel_get_default_config(SIM_FLASH_tx_channel2);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm, false));
    dma_channel_configure(
        SIM_FLASH_tx_channel2,          
        &c,             
        &ssi_hw->dr0,                                       // The initial write address
        &SIM_FLASH_pio_read->rxf[SIM_FLASH_pio_read_sm],    // The initial read address
        1,              // Number of transfers
        false           // Start immediately.
    );
}

static __always_inline void wait_for_cs_high() {
    while (true) {
        if (gpio_get(SIM_FLASH_SPI_CS)) {
                break;
        }
    }
}

/*
    This function need to be run from ram because we need SSI to get some data so we can't use XIP
*/
void __no_inline_not_in_flash_func(handle_read)(){
    ssi_hw->ssienr = 0; // disable SSI
    ssi_hw->ctrlr1 = 3; // NDF, number of data frames
    ssi_hw->dmacr = SSI_DMACR_TDMAE_BITS | SSI_DMACR_RDMAE_BITS; // enable DMA
    ssi_hw->ssienr = 1;


    // We cannot use pico-sdk function because we don't know if they will be inlined of will use XIP
    dma_hw->ch[SIM_FLASH_tx_channel2].ctrl_trig |= DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[SIM_FLASH_tx_channel3].ctrl_trig |= DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[SIM_FLASH_tx_channel].read_addr = emu_ram;

    // wait_for_cs_high();
    while(true){
        if(sio_hw->gpio_in & (1u << SIM_FLASH_SPI_CS))
            break;
    }

    // Reconfigure SSI before we jump back into flash!
    ssi_hw->ssienr = 0;
    ssi_hw->ctrlr1 = 0;
    ssi_hw->dmacr = 0;
    ssi_hw->ssienr = 1;

    dma_channel_abort(SIM_FLASH_tx_channel);

    // Can be used for debug puspose
    // uint32_t bank_addr = pio_sm_get_blocking(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm);
    // printf("Request address: %x\n", bank_addr);
    // printf("%x %x %x %x\n", emu_ram[0], emu_ram[1], emu_ram[2], emu_ram[3]);
}


static void __scratch_x("core1_main") core1_main()
{

    element_t element = {
            .value = 0
    };
    
    wait_for_cs_high();
    goto reset;

    while (true) {
        uint32_t cmd = pio_sm_get_blocking(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm);

        if (cmd == 0x3 || cmd == 0xb) { // Simple Read
            handle_read();
        }
        else if (cmd == 0x9f) {
            // Read Identification
            emu_ram[0] = 0xc8;
            emu_ram[1] = '@';
            emu_ram[2] = 0x16;
            dma_hw->ch[SIM_FLASH_tx_channel].al3_read_addr_trig = emu_ram;

            pio_sm_exec(SIM_FLASH_pio_write,SIM_FLASH_pio_write_sm,pio_encode_jmp(flash_write_wrap_target));

            wait_for_cs_high();
            dma_channel_abort(SIM_FLASH_tx_channel);

        }
        else {
            // Ignore unknown command
            wait_for_cs_high();
        }
reset:
        /* PIO needs to be reset */
        pio_sm_set_enabled(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm, false);
        pio_sm_clear_fifos(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm);
        pio_sm_restart(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm);
        pio_sm_exec(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm, pio_encode_jmp(pio_read_offset));
        pio_sm_set_enabled(SIM_FLASH_pio_read, SIM_FLASH_pio_read_sm, true);   

        pio_sm_set_enabled(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm, false);
        pio_sm_clear_fifos(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm);
        pio_sm_restart(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm);
        pio_sm_exec(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm, pio_encode_jmp(pio_write_offset));
        pio_sm_set_enabled(SIM_FLASH_pio_write, SIM_FLASH_pio_write_sm, true);

#if DEBUG
        element.value = cmd;
        queue_try_add(&sample_fifo, &element);     
#endif
    }
}

void setup_simulated_flash() {
    /* Run core at max stable frequency */
    set_sys_clock_khz(270000, true);

    /* Init the TX and RX pio state machines */
    setup_flash_pio();

    /* Setup the DMA channels */
    setup_rx_channel();
    setup_tx_channel();

    /* This queue is used for debug */
#if DEBUG
    queue_init(&sample_fifo, sizeof(element_t), FIFO_LENGTH);
#endif

    hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS);
    multicore_launch_core1(core1_main);


    return;
}