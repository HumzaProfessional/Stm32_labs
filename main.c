
#include "stm32l476xx.h"
#include "led_setup.h"

/**
 ******************************************8
 * @file main.c
 * @brief main program
 * @ author Humza Rana & Mac
 * Lab 4: Led shifitng blink
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light leds.
 *  Two buttons are enabled to trigger a systick interupt.
 *  A interger called pattern is used to determine what leds
 *  are triggered.
 *
 *
 */

#define SYS_CLK_FREQ 4000000
#define SYSTICK_10HZ   ((SYS_CLK_FREQ / 20) - 1)

// Speed values (reload values for SysTick) for different speeds:
// FAST: ~ (4MHz/10)-1, MEDIUM: ~ (4MHz/20)-1, SLOW: ~ (4MHz/30)-1.
#define SLOW   ((SYS_CLK_FREQ / 20) - 1)
#define MEDIUM ((SYS_CLK_FREQ / 40) - 1)
#define FAST   ((SYS_CLK_FREQ / 60) - 1)

// Create an array of speeds; order is as you wish. For example:
static const uint32_t speeds[] = {SLOW, MEDIUM, FAST};
static volatile int speedIndex = 0;

// Start SysTick
#define START_SYSTICK()     (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)

// Global variables for pattern + direction
static volatile uint8_t ledPattern = 0x01; // start PC8 lit
static volatile uint8_t direction  = 1;    // 1 => left-to-right, 0 => right-to-left
extern volatile uint8_t ledPattern; // allows access to this integer to led_setup


// Forward declaration
void configureSysTick(void);
void SysTick_Handler(void);


int main(void)
{
    // 1) Initialize PC0..PC1 as inputs, PC8..PC15 as outputs
    init_Buttons();          // from led_setup
    init_LEDs_PC8to15();     // from led_setup

    configureSysTick();
        // 4) Start SysTick
    START_SYSTICK();



    while (1)
      {
          // Continuously write the current pattern to the pins
          update_LEDs_PC8to15(ledPattern);
          if (speedIndex >= 3) {
          	       speedIndex = 0;
                      }
          		         // Update the SysTick reload value with the new speed.
          		   SysTick->LOAD = speeds[speedIndex];


      }
  }
// Setup SysTick so it interrupts at ~10Hz
void configureSysTick(void)
{
	SysTick->LOAD = SYSTICK_10HZ;  // Now expands to ((4000000 / 10) - 1)
    SysTick->VAL  = 0;             // clear current count
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | // CPU clock
                    SysTick_CTRL_TICKINT_Msk;    // enable interrupt, but not start


}



	void SysTick_Handler(void)
	{
	    // 1) Read the button input for PC0, PC1 (active-low => 0=pressed, 1=not pressed)
	    uint8_t btn0 = ( (GPIOC->IDR & (1 << 0)) == 0 ) ? 0 : 1;
	    uint8_t btn1 = ( (GPIOC->IDR & (1 << 1)) == 0 ) ? 0 : 1;

	    // 2) If PC0 is pressed => direction=0 => right-to-left
	    if (btn0 == 0) {
	        direction = 0;
	    }
	    // If PC1 is pressed => direction=1 => left-to-right
	    if (btn1 == 0) {
	        direction = 1;
	    }

	    // 3) Shift the pattern based on direction
	    if (direction == 1) {
	        // left to right => shift left
	        ledPattern <<= 1;
	        if (ledPattern == 0) {
	            ledPattern = 0x01; // wrap to PC8
	        }

	    } else {
	        // right to left => shift right
	        ledPattern >>= 1;
	        if (ledPattern == 0) {
	            ledPattern = 0x80; // wrap to PC15
	        }


	    }

	}
