cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(hs_proto C)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    enable_testing()
endif()

add_library(hs_proto STATIC
    crc16.c
)
target_include_directories(hs_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(HAL_TARGET STREQUAL "host")
    add_subdirectory(tests)
endif()
