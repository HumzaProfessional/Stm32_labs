
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
#define SLOW   ((SYS_CLK_FREQ / 6) - 1)
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
    init_LEDs_PC8to15();     // from led_setup

    configureSysTick(); // function in main.c
        // 4) Start SysTick
    START_SYSTICK();

    while (1)
      {
          // Continuously write the current pattern to the pins
          update_LEDs_PC8to15(ledPattern, led_mode);

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
    // Read left button (index left_button) from PC1 and right button (index right_button) from PC0.
    uint8_t leftState  = ((buttons[left_button].port->IDR & (0UL << buttons[left_button].pin)) == 0);
    uint8_t rightState = ((buttons[right_button].port->IDR & (1UL << buttons[right_button].pin)) == 0);

    // If both buttons are pressed (active low: 0 means pressed)
    if (leftState && rightState)
    {
        // Toggle LED mode:
        // For example, if the current mode is MOVE_LED_MODE, switch to FLASH_LED_MODE; otherwise, switch to MOVE_LED_MODE.
        led_mode = (led_mode == SINGLE_LED_MODE) ? FLASH_LED_MODE : SINGLE_LED_MODE;
    }

}
