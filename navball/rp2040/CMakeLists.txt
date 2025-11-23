cmake_minimum_required(VERSION 3.13)

# set(FAMILY rp2040)
# set(BOARD pico_sdk)
# set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_host_")

include(pico_sdk_import.cmake)

project(navball)

pico_sdk_init()

add_executable(navball
    main.c
)

# pico_generate_pio_header(navball ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# inlcude source of tinyusb in PICO_SDK_PATH/lib/tinyusb/src
# target_include_directories(navball PRIVATE ${PICO_SDK_PATH}/lib/tinyusb/src)
target_include_directories(navball PUBLIC ${CMAKE_CURRENT_LIST_DIR})


# Add pico_stdlib library, add more if used
target_link_libraries(navball
                    PUBLIC
                    pico_stdlib
                    pico_unique_id
                    tinyusb_device
                    tinyusb_board
                    hardware_flash
                    pico_multicore
                    hardware_adc
                    hardware_gpio
                    hardware_pio
                    hardware_spi
                    hardware_i2c
                    hardware_clocks
                    hardware_dma
                    )

# enable usb output, disable uart output
pico_enable_stdio_usb(navball 1)
pico_enable_stdio_uart(navball 0)

# Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(navball)
