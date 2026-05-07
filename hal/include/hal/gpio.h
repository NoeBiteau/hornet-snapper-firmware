#ifndef HS_HAL_GPIO_H
#define HS_HAL_GPIO_H

#include <stdbool.h>

typedef int hs_gpio_t;
typedef enum { HS_GPIO_DIR_IN = 0, HS_GPIO_DIR_OUT = 1 } hs_gpio_dir_t;

int  hs_gpio_set_dir(hs_gpio_t pin, hs_gpio_dir_t dir);
int  hs_gpio_write(hs_gpio_t pin, bool level);
bool hs_gpio_read(hs_gpio_t pin);

#endif
