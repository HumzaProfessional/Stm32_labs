#ifndef BUTTONS_H
#define BUTTONS_H

#include "stm32l476xx.h"

// Definitions that organize the button data positions
#define NUM_BUTTONS 3
#define BTN_RIGHT   0
#define BTN_LEFT    1
#define BTN_USER    2

// structure which stores all the buttons
typedef struct {
    uint32_t filter;
    uint32_t state;
    GPIO_TypeDef *port;
    uint32_t pin;
} Button;

// Allows global access to the array
extern volatile Button buttons[NUM_BUTTONS];

// function for initializing buttons
void init_Buttons(void);

#endif
