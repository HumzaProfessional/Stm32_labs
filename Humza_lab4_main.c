

#include "stm32l476xx.h"
#include "led_setup.h"

/**
 ******************************************
 * @file main.c
 * @brief main program
 * @author Humza Rana & Mac
 * @Lab 4:
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light leds in two modes:
 *  Two buttons are enabled to trigger a Systick interrupt.
 *  A integer called pattern is used to determine what leds are triggered.
 *  Systick, defined speeds, and an array are used to determine the
 */

#define SYS_CLK_FREQ 4000000 // determines the frequency of Systick
#define SYSTICK_10HZ   ((SYS_CLK_FREQ / 20) - 1) // Initial frequency rate

// Speed values (reload values for SysTick) for different speeds:
// FAST: ~ (4MHz/8)-1, MEDIUM: ~ (4MHz/12)-1, SLOW: ~ (4MHz/20)-1.
#define SLOW   ((SYS_CLK_FREQ / 5) - 1)
#define MEDIUM ((SYS_CLK_FREQ / 10) - 1)
#define FAST   ((SYS_CLK_FREQ / 15) - 1)

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

#define NUM_BUTTONS 2
#define left_button  0
#define right_button 1

struct button buttons[NUM_BUTTONS] = {
    {0, 0, GPIOC, 1},
    {0, 0, GPIOC, 0}
};




// Start SysTick
#define START_SYSTICK()     (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)

// Global variables for pattern and direction
static volatile uint8_t ledPattern = 0x01; // start PC8 lit
static volatile uint8_t led_mode = SINGLE_LED_MODE;
static volatile uint8_t direction  = 1;
static volatile uint8_t blinkstate;

extern volatile uint8_t led_mode;
extern volatile uint8_t ledPattern; // allows access to this integer to led_setup

// Function declaration
void configureSysTick(void);
void SysTick_Handler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void handleDualButtonPress(void);


int main(void)
{
    // 1) Initialize PC0 and PC1 as inputs, PC8..PC15 as outputs
    init_Buttons();          // from led_setup
    init_LEDs_PC6to13();     // from led_setup

    configureSysTick(); // function in main.c
        // 4) Start SysTick
    START_SYSTICK();

    while (1)
      {
          // Continuously write the current pattern to the pins
          update_LEDs_PC6to13(ledPattern, led_mode);
          // Poll buttons with a delay for debounce.
              if (((GPIOC->IDR & (1UL << 0)) == 0) && ((GPIOC->IDR & (1UL << 1)) == 0))
              {
                  // Both buttons pressed: toggle mode.
                  if (led_mode == SINGLE_LED_MODE)
                      led_mode = FLASH_LED_MODE;
                  else
                      led_mode = SINGLE_LED_MODE;

                  // Wait a little to avoid repeated toggling.
                  for (volatile int i = 0; i < 500000; i++);
              }

      }
  }

/*==================================================================
 * configureSystick()
 *
 * @param: none
 * @return: none
 *
 * Initializes the systick interrupt
 *==================================================================
 */
void configureSysTick(void)
{
	SysTick->LOAD = SYSTICK_10HZ;  // Now expands to ((4000000 / 10) - 1)
    SysTick->VAL  = 0;             // clear current count
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | // CPU clock
                    SysTick_CTRL_TICKINT_Msk;    // enable interrupt, but not start


}

/*==================================================================
 * SysTick_Handler()
 *
 * @param: none
 * @return: none
 *
 * Enables the buttons to interrupt based on whether or not the two buttons are active low.
 * Determines the direction through that logic.
 * Then it checks if the pattern is at the farthest left or right side.
 * If so, the speedIndex is changes to the next speed.
 * Resets once it passed the FAST speed.
 *==================================================================
 */
void SysTick_Handler(void)
{
    // Only update the pattern if we are in SINGLE_LED_MODE.
    if (led_mode == SINGLE_LED_MODE) {
        // Check the right button (PC0) for right-to-left shift.
        if ((GPIOC->IDR & (1UL << 0)) == 0) {
            // PC0 pressed: shift right (i.e., move LED from a lower-numbered pin to a higher-numbered one)
            if (ledPattern == 0x01) {
                // At far left (PC8 lit), wrap to far right (PC15 lit) and cycle speed.
                speedIndex++;
                if (speedIndex >= 3)
                    speedIndex = 0;
                SysTick->LOAD = speeds[speedIndex];
                ledPattern = 0x80;  // PC15 lit
            } else {
                ledPattern >>= 1;
            }
        }

        // Check the left button (PC1) for left-to-right shift.
        if ((GPIOC->IDR & (1UL << 1)) == 0) {
            // PC1 pressed: shift left (i.e., move LED from a higher-numbered pin to a lower-numbered one)
            if (ledPattern == 0x80) {
                // At far right (PC15 lit), wrap to far left (PC8 lit) and cycle speed.
                speedIndex++;
                if (speedIndex >= 3)
                    speedIndex = 0;
                SysTick->LOAD = speeds[speedIndex];
                ledPattern = 0x01;  // PC8 lit
            } else {
                ledPattern <<= 1;
            }
        }
    }
    else if (led_mode == FLASH_LED_MODE) {
            // FLASH_LED_MODE: Toggle between the lower and upper four LEDs.
            if (ledPattern == 0x0F) {
                ledPattern = 0xF0;  // Upper 4 LEDs on (PC12..PC15)
            } else {
                ledPattern = 0x0F;  // Lower 4 LEDs on (PC8..PC11)
            }

            // Adjust flash rate based on button input:
            // If PC0 is pressed, decrease the flash rate.
            if ((GPIOC->IDR & (1UL << 0)) == 0) {
                speedIndex--;
                if (speedIndex < 0) {
                    speedIndex = 2;  // wrap to last index (array has 3 speeds: 0,1,2)
                }
                SysTick->LOAD = speeds[speedIndex];
            }
            // If PC1 is pressed, increase the flash rate.
            if ((GPIOC->IDR & (1UL << 1)) == 0) {
                speedIndex++;
                if (speedIndex > 2) {
                    speedIndex = 0;  // wrap back to first index
                }
                SysTick->LOAD = speeds[speedIndex];
            }
        }
}
// EXTI0_IRQHandler for PC0 (right button)
void EXTI0_IRQHandler(void)
{
    handleDualButtonPress();
}

// EXTI1_IRQHandler for PC1 (left button)
void EXTI1_IRQHandler(void)
{
    handleDualButtonPress();
}

// This function reads the buttons using the 'buttons' struct array
// and toggles led_mode if both buttons are pressed.
void handleDualButtonPress(void)
{
	for(volatile int i = 0; i < 1000; i++)
	{
    // Check PC0 and PC1 directly.
    if (((GPIOC->IDR & (1UL << 0)) == 0) && ((GPIOC->IDR & (1UL << 1)) == 0))
    {
        // Both buttons are pressed (active-low), so toggle led_mode.
        if (led_mode == SINGLE_LED_MODE)
        {
            led_mode = FLASH_LED_MODE;
        }
        else
        {
            led_mode = SINGLE_LED_MODE;
        }
     }
	}
}

