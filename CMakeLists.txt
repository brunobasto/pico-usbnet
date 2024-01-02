cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_tcp C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

# TinyUSB
set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)

# LWIP
set(LWIP_DIR ${PICO_SDK_PATH}/lib/lwip)
set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
include(${LWIP_DIR}/src/Filelists.cmake)

add_executable(${PROJECT_NAME}
    ${PICO_TINYUSB_PATH}/lib/networking/dhserver.c
    ${PICO_TINYUSB_PATH}/lib/networking/rndis_reports.c
    USBNetwork.cpp
    TCP.cpp
    UDP.cpp
    usb_descriptors.c
)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LWIP_INCLUDE_DIRS}
    ${PICO_TINYUSB_PATH}/src
    ${PICO_TINYUSB_PATH}/lib/networking
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    hardware_adc
    pico_stdlib
    pico_unique_id
    pico_lwip
    pico_lwip_arch
    tinyusb_device
    tinyusb_board
    pico_stdlib
    pico_unique_id
    tinyusb_device
    lwipallapps
    lwipcore
)

pico_add_extra_outputs(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_ENTER_USB_BOOT_ON_EXIT=1)
