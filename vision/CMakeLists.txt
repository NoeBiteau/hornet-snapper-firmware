add_library(hs_vision STATIC)
target_include_directories(hs_vision PUBLIC include)
target_link_libraries(hs_vision PUBLIC ${OpenCV_LIBS} onnxruntime nlohmann_json::nlohmann_json hs_proto hs_hal)
target_compile_features(hs_vision PUBLIC cxx_std_17)
# Sources added by later tasks
target_sources(hs_vision PRIVATE src/.gitkeep_placeholder.cpp)

enable_testing()
# Tests added by later tasks
