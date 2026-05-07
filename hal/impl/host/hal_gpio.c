#include "hal/gpio.h"
int  hs_gpio_set_dir(hs_gpio_t p, hs_gpio_dir_t d) { (void)p; (void)d; return 0; }
int  hs_gpio_write(hs_gpio_t p, bool l)             { (void)p; (void)l; return 0; }
bool hs_gpio_read(hs_gpio_t p)                      { (void)p; return false; }
