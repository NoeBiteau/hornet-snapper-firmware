add_executable(test_crc16 test_crc16.c)
target_link_libraries(test_crc16 PRIVATE hs_proto)
add_test(NAME crc16 COMMAND test_crc16)

add_executable(test_uart_frame test_uart_frame.c)
target_link_libraries(test_uart_frame PRIVATE hs_proto)
add_test(NAME uart_frame COMMAND test_uart_frame)

add_executable(test_vectors_roundtrip test_vectors_roundtrip.c)
target_link_libraries(test_vectors_roundtrip PRIVATE hs_proto)
add_test(NAME vectors_roundtrip
         COMMAND test_vectors_roundtrip
                 ${CMAKE_CURRENT_LIST_DIR}/../vectors/v1/uart_frame.json)
