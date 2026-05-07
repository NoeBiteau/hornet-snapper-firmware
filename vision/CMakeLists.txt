add_library(hs_vision STATIC)
target_include_directories(hs_vision PUBLIC include)
target_link_libraries(hs_vision PUBLIC ${OpenCV_LIBS} onnxruntime nlohmann_json::nlohmann_json hs_proto hs_hal)
target_compile_features(hs_vision PUBLIC cxx_std_17)
# Sources added by later tasks
target_sources(hs_vision PRIVATE
    src/video_file_source.cpp)

enable_testing()

# Fixture setup: ensure the synth video is built before any test that needs it
add_test(NAME hs_vision_fixtures_setup
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target hs_vision_fixtures)
set_tests_properties(hs_vision_fixtures_setup PROPERTIES
    FIXTURES_SETUP hs_vision_fixtures_built)

add_executable(test_frame_source_compile tests/test_frame_source_compile.cpp)
target_link_libraries(test_frame_source_compile PRIVATE hs_vision)
add_test(NAME vision.frame_source_compile COMMAND test_frame_source_compile)

add_executable(test_video_file_source tests/test_video_file_source.cpp)
target_link_libraries(test_video_file_source PRIVATE hs_vision)
add_test(NAME vision.video_file_source COMMAND test_video_file_source)
set_tests_properties(vision.video_file_source PROPERTIES
    ENVIRONMENT "HS_FIXTURE_DIR=${HS_VISION_FIXTURE_DIR}"
    FIXTURES_REQUIRED hs_vision_fixtures_built)
