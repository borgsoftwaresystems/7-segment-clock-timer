cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project)
pico_sdk_init()

add_executable(7_segment_clock_timer
        7_segment_clock_timer.cpp
        PicoTM1637.c
        )

pico_enable_stdio_usb(7_segment_clock_timer 1)
pico_enable_stdio_uart(7_segment_clock_timer 0)

add_library(PicoTM1637 INTERFACE)
target_sources(PicoTM1637 INTERFACE
${CMAKE_CURRENT_LIST_DIR}/PicoTM1637.c
)

pico_generate_pio_header(PicoTM1637 ${CMAKE_CURRENT_LIST_DIR}/PicoTM1637.pio)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(7_segment_clock_timer PicoTM1637 pico_stdlib hardware_rtc hardware_pio hardware_pwm)


# create map/bin/hex file etc.
pico_add_extra_outputs(7_segment_clock_timer)
