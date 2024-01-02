cmake_minimum_required(VERSION 3.13)

include(cmake/pico_sdk_import.cmake)

project(usbnet C CXX ASM)
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
    ${LWIP_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lwip
)
include(${LWIP_DIR}/src/Filelists.cmake)

add_library(${PROJECT_NAME}
    ${PICO_TINYUSB_PATH}/lib/networking/dhserver.c
    ${PICO_TINYUSB_PATH}/lib/networking/rndis_reports.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/USBNetwork.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TCP.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UDP.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${LWIP_INCLUDE_DIRS}
    ${PICO_TINYUSB_PATH}/src
    ${PICO_TINYUSB_PATH}/lib/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tusb
    ${CMAKE_CURRENT_SOURCE_DIR}/include/usbnet
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    lwipallapps
    lwipcore
    pico_lwip
    pico_lwip_arch
    pico_stdlib
    pico_unique_id
    tinyusb_board
    tinyusb_device
)
