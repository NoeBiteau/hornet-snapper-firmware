add_library(app_c3_core STATIC
    src/app_context.cpp
    src/cmd_router.cpp
    src/event_bus.cpp
    src/fault_tree.cpp
    src/power_gov.cpp
    src/storage_ring.cpp
    src/watchdog.cpp
    src/fsm/state_machine.cpp)
target_include_directories(app_c3_core PUBLIC src)
target_link_libraries(app_c3_core PUBLIC hs_proto hs_hal)
target_compile_features(app_c3_core PUBLIC cxx_std_17)

add_executable(app_c3 main.cpp)
target_link_libraries(app_c3 PRIVATE app_c3_core)

add_executable(test_c3_fsm_transitions tests/test_c3_fsm_transitions.cpp)
target_link_libraries(test_c3_fsm_transitions PRIVATE app_c3_core hs_proto)
add_test(NAME c3.fsm_transitions COMMAND test_c3_fsm_transitions)

add_executable(test_c3_fire_path tests/test_c3_fire_path.cpp)
target_link_libraries(test_c3_fire_path PRIVATE app_c3_core hs_proto)
add_test(NAME c3.fire_path COMMAND test_c3_fire_path)

add_executable(test_c3_fire_interlock tests/test_c3_fire_interlock.cpp)
target_link_libraries(test_c3_fire_interlock PRIVATE app_c3_core hs_proto)
add_test(NAME c3.fire_interlock COMMAND test_c3_fire_interlock)

add_executable(test_c3_fault_tree tests/test_c3_fault_tree.cpp)
target_link_libraries(test_c3_fault_tree PRIVATE app_c3_core hs_proto)
add_test(NAME c3.fault_tree COMMAND test_c3_fault_tree)

add_executable(test_c3_storage_ring tests/test_c3_storage_ring.cpp)
target_link_libraries(test_c3_storage_ring PRIVATE app_c3_core hs_proto)
add_test(NAME c3.storage_ring COMMAND test_c3_storage_ring)

add_executable(test_c3_watchdog tests/test_c3_watchdog.cpp)
target_link_libraries(test_c3_watchdog PRIVATE app_c3_core hs_proto)
add_test(NAME c3.watchdog COMMAND test_c3_watchdog)

add_executable(test_c3_cmd_router tests/test_c3_cmd_router.cpp)
target_link_libraries(test_c3_cmd_router PRIVATE app_c3_core hs_proto)
add_test(NAME c3.cmd_router COMMAND test_c3_cmd_router)
