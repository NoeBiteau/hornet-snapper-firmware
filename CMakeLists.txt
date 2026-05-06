cmake_minimum_required(VERSION 3.20)
project(hornet_snapper_firmware C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED HAL_TARGET)
    set(HAL_TARGET "host" CACHE STRING "HAL implementation: host | pi5 | c3 | rv1106")
endif()
message(STATUS "HAL_TARGET = ${HAL_TARGET}")

enable_testing()

add_subdirectory(shared/proto)
add_subdirectory(hal/impl/${HAL_TARGET})
add_subdirectory(apps/c3)
add_subdirectory(apps/rv1106)
add_subdirectory(tools/gen_vectors)
