
#include "stm32l476xx.h"
#include "led_setup.h"
#include "buttons.h"

/* ==============================================================================
 * @file: main.c
 * @brief: main program
 * @ author: Humza Rana & Mac
 * @Lab 4: 1D pong using leds
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled and buttons are used to simulate playing pong.
 *  Two buttons are enabled to trigger a Systick interrupt.
 *  This interrupt determines if the player has "swung" or served(if in the serving state)
 *  State machines are used to determine the following: what state the game is in such as serving.
 *
 * 
 * =================================================================================
 */


#define SYS_CLK_FREQ 4000000 // determines the frequency of Systick
#define SYSTICK_10HZ   ((SYS_CLK_FREQ / 20) - 1) // Initial frequency rate

// Speed values (reload values for SysTick) for different speeds:
#define SLOW   ((SYS_CLK_FREQ / 5) - 1)
#define MEDIUM ((SYS_CLK_FREQ / 10) - 1)
#define FAST   ((SYS_CLK_FREQ / 14) - 1)



// Start SysTick
#define START_SYSTICK()     (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)


// Function declaration
void configureSysTick(void);
void SysTick_Handler(void);

static uint32_t msTimer = 0;

//--------------------------------------------------------------------------------
// main()
//---------------------------------------------------------------------------------
int main(void)
{

init_Buttons();
init_LEDs_PC5to12();  //

 configureSysTick();
START_SYSTICK();

uint8_t prevUserBtn = 1;

while (1)
    {
        // Read debounced state from SysTick debounce filter
        uint8_t currUserBtn = buttons[BTN_USER].state;

        // Rising edge detection (button released)
        if (prevUserBtn == 0 && currUserBtn == 1) {
            led_mode = (led_mode == PLAY_MODE) ? FLASH_LED_MODE : PLAY_MODE;

            // Clear playfield
            GPIOC->ODR &= ~(0xFF << 5);  // ✅ PC5–PC12
        }

        prevUserBtn = currUserBtn;

        if (led_mode == PLAY_MODE) {
            playMode();  // game logic, LED shifting, scoring, etc.
        } else if (led_mode == FLASH_LED_MODE) {
            // Optional flashing pattern (e.g. alternate halves)
            static uint32_t blinkTimer = 0;
            if (++blinkTimer > 100000) {
                ledPattern = (ledPattern == 0x0F) ? 0xF0 : 0x0F;
                update_LEDs_PC6to13(ledPattern, led_mode);
                blinkTimer = 0;
            }
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
 * Changes depending on if led_mode is in SINGLE_LED_MODE or FLASH_LED.
 * In SINGLE_LED_MODE, the buttons trigger left or right direction changes of the lit led.
 * If it passed the farthest left or right led, the rate of led
 * Resets once it passed the FAST speed.
 *==================================================================
 */
void SysTick_Handler(void)
{
	    msTimer++;

	    for (int i = 0; i < NUM_BUTTONS; i++)
	    {
	        // Shift in the current raw input value into the filter
	        buttons[i].filter <<= 1U;

	        if (buttons[i].port->IDR & (1U << buttons[i].pin)) {
	            buttons[i].filter |= 1U;
	        }

	        // Button is considered "pressed" if filter is all 0s
	        // Button is considered "released" if filter is all 1s
	        switch (buttons[i].filter)
	        {
	            case 0x00000000:
	                if (buttons[i].state == 1) {
	                    // Button transitioned: Released -> Pressed
	                    // You can add edge-triggered behavior here if needed

	                }
	                buttons[i].state = 0; // Debounced: Pressed
	                break;

	            case 0xFFFFFFFF:
	                buttons[i].state = 1; // Debounced: Released
	                break;

	            default:
	                // Still bouncing — keep previous state
	                break;
	        }
	    }
}
