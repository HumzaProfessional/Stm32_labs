#ifndef LED_SETUP_H
#define LED_SETUP_H

/*************************************************
 * @file: led_setup.h
 *
 * This is the header file for led_setup.c
 * Gives access to the functions in led_setup.c for main.c
 ******************************************************
 */

#include <stdint.h>

// Configure PC0..PC1 as input pull-ups for buttons.
void init_Buttons(void);

// Configure PC8..PC15 as outputs for LEDs.
void init_LEDs_PC8to15(void);

// Write an 8-bit pattern to PC8..PC15 (bit 0 => PC8, bit 7 => PC15).
void update_LEDs_PC8to15(uint8_t pattern);

#endif // LED_SETUP_H
