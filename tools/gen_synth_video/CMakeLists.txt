# firmware/tools/gen_synth_video/CMakeLists.txt
add_executable(gen_synth_video main.cpp)
target_link_libraries(gen_synth_video PRIVATE ${OpenCV_LIBS})
target_compile_features(gen_synth_video PRIVATE cxx_std_17)

set(HS_VISION_FIXTURE_DIR ${CMAKE_BINARY_DIR}/vision_fixtures CACHE INTERNAL "")
file(MAKE_DIRECTORY ${HS_VISION_FIXTURE_DIR})

add_custom_command(
    OUTPUT  ${HS_VISION_FIXTURE_DIR}/synth_blob_30f.mp4
    COMMAND $<TARGET_FILE:gen_synth_video> ${HS_VISION_FIXTURE_DIR}/synth_blob_30f.mp4
    DEPENDS gen_synth_video
    COMMENT "Generating synth_blob_30f.mp4 fixture")

add_custom_target(hs_vision_fixtures ALL
    DEPENDS ${HS_VISION_FIXTURE_DIR}/synth_blob_30f.mp4)
