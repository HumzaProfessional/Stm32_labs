#ifndef BUTTONS_H
#define BUTTONS_H

#include "stm32l476xx.h"

#define NUM_BUTTONS 3
#define BTN_RIGHT   0
#define BTN_LEFT    1
#define BTN_USER    2

typedef struct {
    uint32_t filter;
    uint32_t state;
    GPIO_TypeDef *port;
    uint32_t pin;
} Button;

extern volatile Button buttons[NUM_BUTTONS];

void init_Buttons(void);

#endi
