
import struct, sys

flash_size = int(sys.argv[1])

data = b''.join([struct.pack('>i', 0x41000000+x) for x in range(0, flash_size, 4)])
# data = open('test.bin','rb').read(1024*1024*1)

assert len(data) <= flash_size

template = f"""
#ifndef _FLASH_EMU_DATA_H
#define _FLASH_EMU_DATA_H

#include <pico/platform.h>

volatile uint8_t __attribute__((section(".section_persistent.toto"))) samples [] = {{
    {str(list(data))[1:-1]}
}};

#endif
"""

with open('data.h', 'w') as f:
    f.write(template)