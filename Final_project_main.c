
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
#define SYSTICK_2HZ   ((SYS_CLK_FREQ / 5) - 1)
#define START_SYSTICK() (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)

// Game state
typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_RIGHT_HITZONE,
    STATE_LEFT_HITZONE,
    STATE_RIGHT_HIT,
    STATE_LEFT_HIT,
    STATE_RIGHT_MISS
    
   
} PongState;

static PongState gameState = STATE_SERVE;


static uint32_t msTimer = 0;
volatile uint8_t tickFlag = 0;
uint8_t player1Score = 0;
uint8_t player2Score = 0;



void configureSysTick(void);
void SysTick_Handler(void);


int main(void)
{
    init_Buttons();
    init_LEDs_PC5to12();

    configureSysTick();
    START_SYSTICK();


    serve();  // Start the game

        while (1)
        {
            if (!tickFlag)
                continue;

            tickFlag = 0;

            switch (gameState)
                    {
                        case STATE_SERVE:
                            // Wait for correct player to press button
                            if ((currentServer == 1 && (GPIOC->IDR & (1 << 1)) == 0) ||
                                (currentServer == 0 && (GPIOC->IDR & (1 << 0)) == 0)) {

                                if (ledPattern == 0x01)
                                    gameState = STATE_SHIFT_LEFT;
                                else if (ledPattern == 0x80)
                                    gameState = STATE_SHIFT_RIGHT;
                            }
                            break;

                        case STATE_SHIFT_LEFT:
                            if (ledPattern == 0x80) {
                                // Ball at Player 2's paddle
                                if ((GPIOC->IDR & (1 << 0)) == 0) {
                                    //  Player 2 hit successfully — bounce back
                                    gameState = STATE_SHIFT_RIGHT;
                                } else {
                                    //  Player 2 missed — Player 1 scores
                                    player1Score++;
                                    currentServer = 0;
                                    serve();
                                    gameState = STATE_SERVE;
                                }
                            } else if ((GPIOC->IDR & (1 << 0)) == 0) {
                                //  Player 2 hit too early
                                player1Score++;
                                currentServer = 0;
                                serve();
                                gameState = STATE_SERVE;
                            } else if (!shiftLeft()) {
                                // ❌ Ball went past — Player 1 scores
                                player1Score++;
                                currentServer = 0;
                                serve();
                                gameState = STATE_SERVE;
                            }
                            break;



                        case STATE_SHIFT_RIGHT:
                            if (ledPattern == 0x01) {
                                // Ball at Player 1's paddle
                                if ((GPIOC->IDR & (1 << 1)) == 0) {
                                    // 🎯 Player 1 hit successfully — bounce back
                                    gameState = STATE_SHIFT_LEFT;
                                } else {
                                    // ❌ Player 1 missed
                                    player2Score++;
                                    currentServer = 1;
                                    serve();
                                    gameState = STATE_SERVE;
                                }
                            } else if ((GPIOC->IDR & (1 << 1)) == 0) {
                                // ❌ Player 1 hit too early
                                player2Score++;
                                currentServer = 1;
                                serve();
                                gameState = STATE_SERVE;
                            } else if (!shiftRight()) {
                                // ❌ Ball passed edge
                                player2Score++;
                                currentServer = 1;
                                serve();
                                gameState = STATE_SERVE;
                            }
                            break;


                    }


        }
}

void configureSysTick(void)
{
    SysTick->LOAD = SYSTICK_2HZ;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void)
{

	tickFlag = 1;  // set flag every interval
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
