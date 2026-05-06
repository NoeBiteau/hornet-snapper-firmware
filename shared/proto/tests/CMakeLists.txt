add_executable(test_crc16 test_crc16.c)
target_link_libraries(test_crc16 PRIVATE hs_proto)
add_test(NAME crc16 COMMAND test_crc16)
