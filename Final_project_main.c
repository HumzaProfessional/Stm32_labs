
#include "stm32l476xx.h"
#include "led_setup.h"
#include "buttons.h"

/**
 ******************************************
 * @file main.c
 * @brief main program
 * @author Humza Rana & Mac
 * @Lab 4:
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light leds in two modes: SINGLE_LED_MODE and FLASH_LEDMODE
 *  Two buttons are enabled to trigger a Systick interrupt and EXTI interupts for both buttons.
 *  A integer called pattern is used to determine what leds are lit depeding on the mode.
 *  Systick, defined speeds, and an array are used to determine the speed of frequency of events.
 * A form of debouncing is used to resolve noise between button presses.
 */


#define SYS_CLK_FREQ 4000000
#define SYSTICK_10HZ   ((SYS_CLK_FREQ / 20) - 1)
#define START_SYSTICK() (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)

// Game state
typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_CHECK_LEFT_HIT,
    STATE_CHECK_RIGHT_HIT,
    STATE_WIN
} GameState;

static GameState gameState = STATE_SERVE;


static uint32_t msTimer = 0;

void configureSysTick(void);
void SysTick_Handler(void);


int main(void)
{
    init_Buttons();
    init_LEDs_PC5to12();

    configureSysTick();
    START_SYSTICK();


  while(1){
    	 // Shift left to PC12
    	        while (shiftLeft()) {

    	        }

    	        // Shift right back to PC5
    	        while (shiftRight()) {

    	        }
  }
}

void configureSysTick(void)
{
    SysTick->LOAD = SYSTICK_10HZ;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void)
{


    msTimer++;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].filter <<= 1U;
        if (buttons[i].port->IDR & (1U << buttons[i].pin))
            buttons[i].filter |= 1U;

        switch (buttons[i].filter) {
            case 0x00:
                buttons[i].state = 0;
                break;
            case 0xFF:
                buttons[i].state = 1;
                break;
            default:
                break;
        }
    }
}
