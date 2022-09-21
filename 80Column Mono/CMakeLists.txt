cmake_minimum_required(VERSION 3.14)

#set(PICO_BOARD "vgaboard")

include(pico_sdk_import.cmake)       
include(pico_extras_import.cmake)


pico_sdk_init()

project(picoterm VERSION 1.0.0)
add_executable(picoterm
               main.c
               picoterm.c
               #font6.c
               font8.c
               #font10.c
               #lcd.c
               )



pico_enable_stdio_usb(picoterm 0)
pico_enable_stdio_uart(picoterm 0)

target_include_directories(picoterm PRIVATE ${CMAKE_CURRENT_LIST_DIR})
# I think the above is necessary to include tusb_config.h

pico_add_extra_outputs(picoterm)

target_compile_definitions(picoterm PRIVATE
PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=4
PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA=true 
)

target_link_libraries(picoterm
                      pico_scanvideo_dpi
                      pico_multicore
                      pico_stdlib
                      hardware_gpio
                      hardware_i2c
                      hardware_adc
                      hardware_uart
                      hardware_irq
                      hardware_flash

                      tinyusb_device
                      tinyusb_board
                      tinyusb_host
                      )
