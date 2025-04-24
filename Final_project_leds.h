#ifndef LED_SETUP_H
#define LED_SETUP_H

/*************************************************
 * @file: led_setup.h
 *
 * Header file for led_setup.c
 * Provides LED setup and control functions used in main.c
 *************************************************/

#include <stdint.h>

// LED modes (used in main to toggle between modes)
#define PLAY_MODE 0
#define FLASH_LED_MODE 1

// Global LED state variables (defined in led_setup.c)
extern volatile uint8_t ledPattern;
extern volatile uint8_t led_mode;
extern volatile uint8_t currentServer;

// Initialization for PC5–PC12 and score LEDs
void init_LEDs_PC5to12(void);

// Update the main playfield LEDs with current ledPattern
void update_LEDs_PC5to12(void);

// LED shifting functions for game logic
int shiftRight(void);
int shiftLeft(void);

// Game logic: serve from the correct player side
void serve(void);

// Score tracking and visual feedback
void updatePlayerScore(uint8_t score, uint8_t player);
void flashWinnerScore(uint8_t winner);

// Pattern accessors for FLASH_LED_MODE
uint8_t getCurrentLedPattern(void);
void setLedPattern(uint8_t pattern);

#endif


#endif
