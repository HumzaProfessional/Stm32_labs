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

// LED modes (used in main to toggle between modes)
#define PLAY_MODE 0
#define FLASH_LED_MODE 1

// Global LED state variables (defined in led_setup.c)
extern volatile uint8_t ledPattern;
extern volatile uint8_t led_mode;
extern volatile uint8_t currentServer;

// Initialization
void init_LEDs_PC5to12(void);

// Update playfield LEDs
void update_LEDs_PC5to12();

// LED shifting functions (called by main's state machine)
int shiftRight(void);
int shiftLeft(void);

// Game feedback functions
void serve(void);  // Draws the current server's LED

void updatePlayerScore(uint8_t score, uint8_t player);

uint8_t getCurrentLedPattern(void);
void setLedPattern(uint8_t pattern);


#endif
