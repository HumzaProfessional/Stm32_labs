
#include "stm32l476xx.h"
#include "led_setup.h"

/**
 ******************************************8
 * @file main.c
 * @brief main program
 * @ author Humza Rana & Mac
 * @Lab 4:
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light leds.
 *  Two buttons are enabled to trigger a Systick interrupt.
 *  A integer called pattern is used to determine what leds are triggered.
 *  Systick, defined speeds, and an array are used to determine the
 */

#define SYS_CLK_FREQ 4000000 // determines the frequency of Systick
#define SYSTICK_10HZ   ((SYS_CLK_FREQ / 20) - 1) // Initial frequency rate

// Speed values (reload values for SysTick) for different speeds:
// FAST: ~ (4MHz/8)-1, MEDIUM: ~ (4MHz/12)-1, SLOW: ~ (4MHz/20)-1.
#define SLOW   ((SYS_CLK_FREQ / 8) - 1)
#define MEDIUM ((SYS_CLK_FREQ / 12) - 1)
#define FAST   ((SYS_CLK_FREQ / 20) - 1)

// Create an array of speeds
static const uint32_t speeds[] = {SLOW, MEDIUM, FAST};
static volatile int speedIndex = 0; // variable used for determined the speed


struct button
{
	uint32_t filter;
	uint32_t state;
	GPIO_TypeDef* port;
	uint32_t pin;
};
#define NUM_BUTTONS 1
#define left_button
#define right_button

// struct button buttons[NUM_BUTTONS] = { {0,0. GPIOC, 1}, {0,0, GPIOC, 0}};
