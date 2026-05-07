add_library(hs_vision STATIC)
target_include_directories(hs_vision PUBLIC include)
target_link_libraries(hs_vision PUBLIC ${OpenCV_LIBS} onnxruntime nlohmann_json::nlohmann_json hs_proto hs_hal)
target_compile_features(hs_vision PUBLIC cxx_std_17)
# Sources added by later tasks
target_sources(hs_vision PRIVATE
    src/video_file_source.cpp
    src/motion_gate.cpp
    src/mock_detector.cpp
    src/onnx_detector.cpp
    src/tracker.cpp
    src/confirmer.cpp
    src/clip_writer.cpp
    src/event_log.cpp)

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

add_executable(test_motion_gate tests/test_motion_gate.cpp)
target_link_libraries(test_motion_gate PRIVATE hs_vision)
add_test(NAME vision.motion_gate COMMAND test_motion_gate)
set_tests_properties(vision.motion_gate PROPERTIES
    ENVIRONMENT "HS_FIXTURE_DIR=${HS_VISION_FIXTURE_DIR}"
    FIXTURES_REQUIRED hs_vision_fixtures_built)

add_executable(test_mock_detector tests/test_mock_detector.cpp)
target_link_libraries(test_mock_detector PRIVATE hs_vision)
add_test(NAME vision.mock_detector COMMAND test_mock_detector)

add_executable(test_onnx_detector tests/test_onnx_detector.cpp)
target_link_libraries(test_onnx_detector PRIVATE hs_vision)
add_test(NAME vision.onnx_detector COMMAND test_onnx_detector)
set_tests_properties(vision.onnx_detector PROPERTIES
    SKIP_RETURN_CODE 77)

add_executable(test_tracker tests/test_tracker.cpp)
target_link_libraries(test_tracker PRIVATE hs_vision)
add_test(NAME vision.tracker COMMAND test_tracker)

add_executable(test_confirmer tests/test_confirmer.cpp)
target_link_libraries(test_confirmer PRIVATE hs_vision)
add_test(NAME vision.confirmer COMMAND test_confirmer)

add_executable(test_clip_writer tests/test_clip_writer.cpp)
target_link_libraries(test_clip_writer PRIVATE hs_vision)
add_test(NAME vision.clip_writer COMMAND test_clip_writer)

add_executable(test_event_log tests/test_event_log.cpp)
target_link_libraries(test_event_log PRIVATE hs_vision)
add_test(NAME vision.event_log COMMAND test_event_log)
