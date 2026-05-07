#include "hal/uart.h"
int hs_uart_open(int id, uint32_t baud)                       { (void)id; (void)baud; return 0; }
int hs_uart_write(hs_uart_port_t p, const uint8_t *b, size_t n){ (void)p; (void)b; return (int)n; }
int hs_uart_read(hs_uart_port_t p, uint8_t *b, size_t c, int t){ (void)p; (void)b; (void)c; (void)t; return 0; }
