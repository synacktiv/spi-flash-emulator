#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include "hardware/structs/ssi.h"
#include "hardware/sync.h"
#include "pico/util/queue.h"

#include "flash.h"
#include "data.h"

/* Check if core1 sent a message and displays it */
void check_queue(){
    element_t element;
    while (queue_try_remove(&sample_fifo, &element)) {
        printf("Got msg %x\n", element.value);
    }
}

int main() {
    stdio_init_all();

	/* Launch core1 and setup communication */
    setup_simulated_flash();

	/* This line is needed otherwsise the loader doesn't get "samples" in the firmware */
    if (samples[0]){}

    while (1){
		/* Handle messages as soon as possible to avoid queue blocking */
        check_queue();
    }
}