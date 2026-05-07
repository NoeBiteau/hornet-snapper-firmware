add_library(hs_hal STATIC
    hal_gpio.c
    hal_uart.c
    hal_time.c
    hal_motor.c
    hal_encoder.c
)
target_include_directories(hs_hal PUBLIC ${CMAKE_SOURCE_DIR}/hal/include)
