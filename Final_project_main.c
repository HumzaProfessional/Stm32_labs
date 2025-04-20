
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
static uint8_t ballServed = 0;
static uint8_t currentServer = 1;  // 1 = Player 1 (left), 0 = Player 2 (right)
static uint8_t player1Score = 0;
static uint8_t player2Score = 0;
static uint8_t winner = 0;
static uint32_t msTimer = 0;

void configureSysTick(void);
void SysTick_Handler(void);
void playMode(void);

int main(void)
{
    init_Buttons();
    init_LEDs_PC5to12();

    configureSysTick();
    START_SYSTICK();

    uint8_t prevUserBtn = 1;
    uint8_t currUserBtn;

    if (led_mode == PLAY_MODE)
        GPIOA->ODR |= (1 << 5);
    else
        GPIOA->ODR &= ~(1 << 5);

    while (1)
    {
        currUserBtn = buttons[BTN_USER].state;

        if (prevUserBtn == 0 && currUserBtn == 1) {
            if (led_mode == PLAY_MODE) {
                led_mode = FLASH_LED_MODE;
                GPIOA->ODR &= ~(1 << 5);
            } else {
                led_mode = PLAY_MODE;
                GPIOA->ODR |= (1 << 5);
            }
            GPIOC->ODR &= ~(0xFF << 5);
        }

        prevUserBtn = currUserBtn;

        if (led_mode == PLAY_MODE) {
            playMode();
        } else if (led_mode == FLASH_LED_MODE) {
            static uint32_t blinkTimer = 0;
            if (++blinkTimer > 100000) {
                if (ledPattern == 0x0F)
                    ledPattern = 0xF0;
                else
                    ledPattern = 0x0F;

                update_LEDs_PC5to12(ledPattern, led_mode);
                blinkTimer = 0;
            }
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
            case 0x00000000:
                buttons[i].state = 0;
                break;
            case 0xFFFFFFFF:
                buttons[i].state = 1;
                break;
            default:
                break;
        }
    }
    // TEST: Light up PA5 (user LED) when left button is pressed
       if (buttons[BTN_LEFT].state == 0) {
           GPIOA->ODR |= (1 << 5);
       } else {
           GPIOA->ODR &= ~(1 << 5);
       }
}

void playMode(void)
{
    switch (gameState)
    {
        case STATE_SERVE:
            if (!ballServed) {
                if (currentServer == 1)
                    ledPattern = 0x01;
                else
                    ledPattern = 0x80;

                update_LEDs_PC5to12(ledPattern, led_mode);
                ballServed = 1;
            }

            if (currentServer == 1 && buttons[BTN_LEFT].state == 0)
                gameState = STATE_SHIFT_RIGHT;
            else if (currentServer == 0 && buttons[BTN_RIGHT].state == 0)
                gameState = STATE_SHIFT_LEFT;
            break;

        case STATE_SHIFT_RIGHT:
            if (moveRight() == 0)
                gameState = STATE_CHECK_RIGHT_HIT;
            break;

        case STATE_SHIFT_LEFT:
            if (moveLeft() == 0)
                gameState = STATE_CHECK_LEFT_HIT;
            break;

        case STATE_CHECK_RIGHT_HIT:
            if (buttons[BTN_RIGHT].state == 0) {
                gameState = STATE_SHIFT_LEFT;
            } else {
                player1Score++;
                displayPlayerScore(player1Score, 1);
                if (player1Score >= 3) {
                    winner = 1;
                    gameState = STATE_WIN;
                } else {
                    ballServed = 0;
                    currentServer = 0;
                    gameState = STATE_SERVE;
                }
            }
            break;

        case STATE_CHECK_LEFT_HIT:
            if (buttons[BTN_LEFT].state == 0) {
                gameState = STATE_SHIFT_RIGHT;
            } else {
                player2Score++;
                displayPlayerScore(player2Score, 1);
                if (player2Score >= 3) {
                    winner = 2;
                    gameState = STATE_WIN;
                } else {
                    ballServed = 0;
                    currentServer = 1;
                    gameState = STATE_SERVE;
                }
            }
            break;

        case STATE_WIN:
            for (int i = 0; i < 3; i++) {
                if (winner == 1)
                    GPIOA->ODR |= (1 << 13) | (1 << 14) | (1 << 15);
                else
                    GPIOC->ODR |= (1 << 2) | (1 << 3) | (1 << 15);

                for (volatile int d = 0; d < 500000; d++);

                if (winner == 1)
                    GPIOA->ODR &= ~((1 << 13) | (1 << 14) | (1 << 15));
                else
                    GPIOC->ODR &= ~((1 << 2) | (1 << 3) | (1 << 15));

                for (volatile int d = 0; d < 500000; d++);
            }

            player1Score = 0;
            player2Score = 0;
            winner = 0;
            ballServed = 0;
            currentServer = 1;
            displayPlayerScore(0, 1);
            displayPlayerScore(0, 2);
            gameState = STATE_SERVE;
            break;
    }
}



