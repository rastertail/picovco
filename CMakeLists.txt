cmake_minimum_required(VERSION 3.13)

include(3rdparty/pico-sdk/pico_sdk_init.cmake)

project(picovco)
pico_sdk_init()

add_executable(picovco src/picovco.c src/usb_descriptors.c)
target_include_directories(picovco PRIVATE src)
pico_generate_pio_header(picovco ${CMAKE_CURRENT_LIST_DIR}/src/gpio.pio)
target_link_libraries(picovco hardware_gpio hardware_interp hardware_pio pico_stdlib pico_multicore tinyusb_board tinyusb_device)
pico_add_extra_outputs(picovco)