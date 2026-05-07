include(FetchContent)
set(ORT_VERSION 1.17.3)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ORT_PKG onnxruntime-linux-aarch64-${ORT_VERSION}.tgz)
    set(ORT_HASH SHA256=9f801577bd99676d1d821022e52b1f4554f56339ae3606c7b5ff3155f443c921)
  else()
    set(ORT_PKG onnxruntime-linux-x64-${ORT_VERSION}.tgz)
    set(ORT_HASH SHA256=f2f11f9da1e3e19b22a8b378b9af57a58433f40e3db6a803e75c0ec0eba97a20)
  endif()
else()
  message(FATAL_ERROR "HS_BUILD_VISION currently supports Linux only. Disable with -DHS_BUILD_VISION=OFF on Windows host.")
endif()
FetchContent_Declare(onnxruntime
  URL https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${ORT_PKG}
  URL_HASH ${ORT_HASH})
FetchContent_MakeAvailable(onnxruntime)
add_library(onnxruntime SHARED IMPORTED GLOBAL)
set_target_properties(onnxruntime PROPERTIES
  IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so
  INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_SOURCE_DIR}/include)
