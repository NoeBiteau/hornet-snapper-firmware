cmake_minimum_required(VERSION 3.20)
project(hornet_snapper_firmware C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

option(HS_BUILD_VISION    "Build firmware/vision library and tests" ON)
option(HS_VISION_LIBCAMERA "Build libcamera live FrameSource (Linux+Pi)" OFF)

if(HS_BUILD_VISION)
  find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio video tracking dnn)
  include(FetchContent)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
  FetchContent_MakeAvailable(json)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/FetchOnnxRuntime.cmake)
  add_subdirectory(tools/gen_synth_video)
  add_subdirectory(vision)
endif()
