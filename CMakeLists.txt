cmake_minimum_required(VERSION 3.13)

set(PROGRAM_NAME DiRPi_Hat_Sending)
set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(DiRPi_Hat_Sending)

pico_sdk_init()


set(SOURCES
    main.c
    hw_config.c
    util.c
    net_util.c
    settings.c
    ws.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROGRAM_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
    pico_util
    hardware_i2c   
    FatFs_SPI
    hardware_rtc
    pico_cyw43_arch_none 
)


pico_enable_stdio_usb(${PROGRAM_NAME} TRUE)
pico_enable_stdio_uart(${PROGRAM_NAME} FALSE)

pico_add_extra_outputs(${PROGRAM_NAME})