#ifndef HS_HAL_UART_H
#define HS_HAL_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int hs_uart_port_t;

int hs_uart_open(int port_id, uint32_t baud);
int hs_uart_write(hs_uart_port_t port, const uint8_t *buf, size_t len);
int hs_uart_read(hs_uart_port_t port, uint8_t *buf, size_t cap, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
