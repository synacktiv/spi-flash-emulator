#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pio_spi.h"
#include <hardware/clocks.h>

#undef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 4

#undef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 3

#undef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 2

#undef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 5


pio_spi_inst_t spi = {
        .pio = pio0,
        .sm = 0,
        .cs_pin = PICO_DEFAULT_SPI_CSN_PIN
};

void test_accuracy(){
    uint32_t target_addr = 0x010004,addr_max = 0x20000, received_addr;
    uint32_t i=0;
    volatile int j = 0;
    char dst[10] = {0};

    while (!stdio_usb_connected()) {}
    puts("Starting tests in 1s");
    sleep_ms(1000);

    int progress  = 0;
    int miss = 0;

    for(target_addr=0; target_addr< addr_max;target_addr+=4  ){


        if((target_addr*10)/addr_max != progress){
            progress = (target_addr*10)/addr_max;
            printf("Progress: %d%%\n", progress*10);
        }

        uint8_t cmd[8] = {
                0x3,
                target_addr >> 16,
                target_addr >> 8,
                target_addr,
                0, 0, 0, 0
        };

        gpio_put(spi.cs_pin, 0);
        pio_spi_write8_read8_blocking(&spi, cmd, dst, 8);
        // pio_spi_read8_blocking(&spi, page_buf, 4);
        gpio_put(spi.cs_pin, 1);


        for (j = 0; j < 100;){
            asm("nop");
            j++;
        }

        if (target_addr >= 1024*1024){
            i = 0;
        }

        received_addr = (dst[4] << 8*3) | (dst[5] << 8*2) | (dst[6] << 8*1) | (dst[7] << 8*0);
        if (received_addr != target_addr){
            miss++;
            printf("%x %x\n", received_addr, target_addr);
        }

    }
    printf("Got %d misses -> accuracy=%d%%\n", miss, 100 - (miss*100/(target_addr/4)));
}

void test_many_reads(){

    uint32_t target_addr = 0x010004;
    uint32_t target_addr2 = 0x020004;
    uint32_t i=1;
    volatile int j = 0;

    while(1){
        target_addr = i*4;
        i+=0x10000/4;

        // flash_sector_erase(&spi, target_addr);
        // flash_read(&spi, target_addr, page_buf, 8);
        uint8_t cmd[8] = {
                0x3,
                target_addr >> 16,
                target_addr >> 8,
                target_addr,
                0, 0, 0, 0
        };

        gpio_put(spi.cs_pin, 0);
        pio_spi_write8_blocking(&spi, cmd, 9);
        // pio_spi_read8_blocking(&spi, page_buf, 4);
        gpio_put(spi.cs_pin, 1);

        // sleep_ms(1);

        for (j = 0; j < 2;){
            asm("nop");
            j++;
        }

        if (target_addr >= 1024*1024){
            i = 0;
        }

        // sleep_ms(1000);

    }
}

int main() {
    stdio_init_all();

    // set_sys_clock_khz(270000, true);

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_pull_down(PICO_DEFAULT_SPI_CSN_PIN);

    uint offset = pio_add_program(spi.pio, &spi_cpha0_program);
    printf("Loaded program at %d\n", offset);

    pio_spi_init(spi.pio, spi.sm, offset,
                 8,       // 8 bits per SPI frame
                 3.525f,  // 1 MHz @ 125 clk_sys
                 false,   // CPHA = 0
                 false,   // CPOL = 0
                 PICO_DEFAULT_SPI_SCK_PIN,
                 PICO_DEFAULT_SPI_TX_PIN,
                 PICO_DEFAULT_SPI_RX_PIN
    );
    // Make the 'SPI' pins available to picotool
    bi_decl(bi_4pins_with_names(PICO_DEFAULT_SPI_RX_PIN, "SPI RX", PICO_DEFAULT_SPI_TX_PIN, "SPI TX", PICO_DEFAULT_SPI_SCK_PIN, "SPI SCK", PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    // test_accuracy();
    test_many_reads();
    

    return 0;
}
