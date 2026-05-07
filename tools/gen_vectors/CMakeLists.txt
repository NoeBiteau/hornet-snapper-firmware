if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(gen_vectors C)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../shared/proto hs_proto_build)
endif()

add_executable(gen_vectors main.c)
target_link_libraries(gen_vectors PRIVATE hs_proto)
