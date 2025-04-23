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
 *  A form of debouncing is used to resolve noise between button presses.
 */

// === Configuration ===
#define SYS_CLK_FREQ       4000000
#define MAX_SPEED_TICKS    100000
#define SPEED_STEP         100000
#define INITIAL_SPEED      400000

typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_RIGHT_HITZONE,
    STATE_RIGHT_HIT,
    STATE_RIGHT_MISS,
    STATE_LEFT_HITZONE,
    STATE_LEFT_HIT,
    STATE_LEFT_MISS
} PongState;

static PongState gameState = STATE_SERVE;
static uint8_t player1Score = 0;
static uint8_t player2Score = 0;
uint32_t currentSpeed = INITIAL_SPEED;
static int hitWaitTicks = 0;
uint32_t msTimer = 0;

void configureSysTick(uint32_t reloadValue);
void SysTick_Handler(void);
void handleFlashLedMode(void);

int main(void)
{
    init_Buttons();
    init_LEDs_PC5to12();
    configureSysTick(currentSpeed);
    serve();
// Ensure the correct initial state of the user LED
    if (led_mode == PLAY_MODE)
    {
        GPIOA->ODR |= GPIO_ODR_OD5;   // Turn ON PA5
    }
    else
    {
        GPIOA->ODR &= ~GPIO_ODR_OD5;  // Turn OFF PA5
    }
 uint8_t prevUserBtn = 1;
  uint8_t currUserBtn;

    while (1)
    {
        // Read user button state directly from PC13
     if (GPIOC->IDR & (1 << 13))
            currUserBtn = 1;
      else
            currUserBtn = 0;

        // Detect rising edge: button released
        if (prevUserBtn == 0 && currUserBtn == 1)
        {
            if (led_mode == PLAY_MODE)
            {
                led_mode = FLASH_LED_MODE;
                GPIOA->ODR &= ~GPIO_ODR_OD5;  // Turn OFF PA5
            }
            else
            {
                led_mode = PLAY_MODE;
                GPIOA->ODR |= GPIO_ODR_OD5;   // Turn ON PA5
            }

            // Optional: Clear PC5–PC12 LEDs
            GPIOC->ODR &= ~(0xFF << 5);
        }

        prevUserBtn = currUserBtn;
        if (led_mode == FLASH_LED_MODE)
        {
            handleFlashLedMode();
        }
    }
}

void configureSysTick(uint32_t reloadValue)
{
    SysTick->LOAD  = reloadValue - 1;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    msTimer++;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].filter <<= 1U;
        if (buttons[i].port->IDR & (1U << buttons[i].pin))
            buttons[i].filter |= 1U;

        buttons[i].filter &= 0xFF;

        switch (buttons[i].filter) {
            case 0x00: buttons[i].state = 0; break;
            case 0xFF: buttons[i].state = 1; break;
            default: break;
        }
    }

    switch (gameState)
    {
        case STATE_SERVE:
            serve();
            if ((currentServer == 1 && buttons[BTN_LEFT].state == 0) ||
                (currentServer == 0 && buttons[BTN_RIGHT].state == 0)) {
                if (ledPattern == 0x01)
                    gameState = STATE_SHIFT_LEFT;
                else if (ledPattern == 0x80)
                    gameState = STATE_SHIFT_RIGHT;
            }
            break;

        case STATE_SHIFT_LEFT:
            if (!shiftLeft() && ledPattern == 0x80) {
                gameState = STATE_RIGHT_HITZONE;
                hitWaitTicks = 0;
            }
            break;

        case STATE_SHIFT_RIGHT:
            if (!shiftRight() && ledPattern == 0x01) {
                gameState = STATE_LEFT_HITZONE;
                hitWaitTicks = 0;
            }
            break;

        case STATE_RIGHT_HITZONE:
            hitWaitTicks++;
            if (buttons[BTN_RIGHT].state == 0) {
                gameState = STATE_RIGHT_HIT;
            } else if (hitWaitTicks > 3) {
                gameState = STATE_RIGHT_MISS;
            }
            break;

        case STATE_LEFT_HITZONE:
            hitWaitTicks++;
            if (buttons[BTN_LEFT].state == 0) {
                gameState = STATE_LEFT_HIT;
            } else if (hitWaitTicks > 3) {
                gameState = STATE_LEFT_MISS;
            }
            break;

        case STATE_RIGHT_HIT:
            if (currentSpeed > MAX_SPEED_TICKS + SPEED_STEP)
                currentSpeed -= SPEED_STEP;
            configureSysTick(currentSpeed);
            gameState = STATE_SHIFT_RIGHT;
            break;

        case STATE_LEFT_HIT:
            if (currentSpeed > MAX_SPEED_TICKS + SPEED_STEP)
                currentSpeed -= SPEED_STEP;
            configureSysTick(currentSpeed);
            gameState = STATE_SHIFT_LEFT;
            break;

        case STATE_RIGHT_MISS:
            player1Score++;
            updatePlayerScore(player1Score, 1);  //
            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 0;
            serve();
            gameState = STATE_SERVE;
            break;

        case STATE_LEFT_MISS:
            player2Score++;
            updatePlayerScore(player2Score, 2);  
            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 1;
            serve();
            gameState = STATE_SERVE;
            break;
    }
   
}

}
void handleFlashLedMode(void)
{
    static uint8_t prevLeftBtn = 1;
    static uint8_t prevRightBtn = 1;

    uint8_t currLeftBtn = buttons[BTN_LEFT].state;
    uint8_t currRightBtn = buttons[BTN_RIGHT].state;

    uint8_t currentPattern = getCurrentLedPattern();

    // Left button released
    if (prevLeftBtn == 0 && currLeftBtn == 1) {
        if (currentPattern == 0x80) {
            setLedPattern(0x01); // Wrap around
        } else {
            setLedPattern(currentPattern << 1);
        }
    }

    // Right button released
    if (prevRightBtn == 0 && currRightBtn == 1) {
        if (currentPattern == 0x01) {
            setLedPattern(0x80); // Wrap around
        } else {
            setLedPattern(currentPattern >> 1);
        }
    }

    prevLeftBtn = currLeftBtn;
    prevRightBtn = currRightBtn;
}
