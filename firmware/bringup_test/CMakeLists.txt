# CTEC bring-up test: YL-38 thermistor on ADC + TEC PWM/DIR output

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(bringup_test C CXX ASM)

pico_sdk_init()

add_executable(bringup_test bringup_test.c)

target_link_libraries(bringup_test
    pico_stdlib
    hardware_adc
    hardware_pwm
)

# printf over USB, not UART
pico_enable_stdio_usb(bringup_test 1)
pico_enable_stdio_uart(bringup_test 0)

pico_add_extra_outputs(bringup_test)
