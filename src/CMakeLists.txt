cmake_minimum_required(VERSION 3.13)

if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

# Set the Pico board
set(PICO_BOARD pico)

# Set the project name
set(PROJECT_NAME flash_emulator)

set(PICO_USE_FASTEST_SUPPORTED_CLOCK 1)

math(EXPR EMU_FLASH_SIZE "1024 * 1024*1")
math(EXPR REAL_FLASH_SIZE "1024 * 1024 * 4")

# Include the Pico SDK. Must be before project()
include(${PICO_SDK_PATH}/pico_sdk_init.cmake
)

# Set the project name
project(${PROJECT_NAME} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# create data array
add_custom_command(
        OUTPUT data.h
        COMMAND python3 "gen_array.py" ${EMU_FLASH_SIZE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/)

add_executable(${PROJECT_NAME}
        main.c
        flash.c
        data.h
)


add_definitions(-DEMU_FLASH_SIZE=${EMU_FLASH_SIZE} -DREAL_FLASH_SIZE=${REAL_FLASH_SIZE})
target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--defsym=__PERSISTENT_STORAGE_LEN=${EMU_FLASH_SIZE}")

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/flash.pio)

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
)

set_target_properties(${PROJECT_NAME} PROPERTIES PICO_TARGET_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/flash_memmap.ld)
pico_add_link_depend(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/flash_memmap.ld)

# Enable USB output (optional)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})