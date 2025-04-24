#include "stm32l476xx.h"
#include "led_setup.h"
#include "buttons.h"

/**
 ******************************************
 * @file main.c
 * @brief Main program file for 1D Pong game
 * @author Humza Rana & Mac
 * @Lab: Finsl Project
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light LEDs in two modes:
 *  PLAY_MODE and FLASH_LED_MODE.
 *  Two buttons are used to interact with the game and SysTick is
 *  used for regular timing.
 *  The user button toggles between modes. Debouncing is used.
 ******************************************
 */

// === Configuration ===
#define SYS_CLK_FREQ       4000000
#define MAX_SPEED_TICKS    100000
#define SPEED_STEP         100000
#define INITIAL_SPEED      400000

#define FLASH_MODE_SPEED 20000  // ~5ms tick = 200Hz (4MHz / 20000)

// === Game States for PLAY_MODE ===
typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_RIGHT_HITZONE,
    STATE_RIGHT_HIT,
    STATE_RIGHT_MISS,
    STATE_LEFT_HITZONE,
    STATE_LEFT_HIT,
    STATE_LEFT_MISS,
    STATE_WIN
} PongState;

// === Global Variables ===
static PongState gameState = STATE_SERVE;
static uint8_t player1Score = 0;
static uint8_t player2Score = 0;
uint32_t currentSpeed = INITIAL_SPEED;
static int hitWaitTicks = 0;
uint32_t msTimer = 0;

// === Function Prototypes ===
void configureSysTick(uint32_t reloadValue);
void SysTick_Handler(void);
void handleFlashLedMode(void);

/**
 * @brief Main entry point
 * Initializes hardware, sets initial game state, and handles user mode switching.
 */
int main(void)
{
    init_Buttons();
    init_LEDs_PC5to12();
    configureSysTick(currentSpeed);  // Set initial speed
    serve();

    // Ensure the correct initial state of the user LED
   if (led_mode == PLAY_MODE)
{
    // Edge detection for button release (debounced)
    static uint8_t prevLeftBtn = 1;
    static uint8_t prevRightBtn = 1;

    uint8_t currLeftBtn = buttons[BTN_LEFT].state;
    uint8_t currRightBtn = buttons[BTN_RIGHT].state;

    uint8_t leftReleased = (prevLeftBtn == 0 && currLeftBtn == 1);
    uint8_t rightReleased = (prevRightBtn == 0 && currRightBtn == 1);

    switch (gameState)
    {
        case STATE_SERVE:
            serve();
            if ((currentServer == 1 && currLeftBtn == 0) ||
                (currentServer == 0 && currRightBtn == 0)) {
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
            if (rightReleased && ledPattern == 0x80) {
                gameState = STATE_RIGHT_HIT;
            } else if (hitWaitTicks > 3) {
                gameState = STATE_RIGHT_MISS;
            }
            break;

        case STATE_LEFT_HITZONE:
            hitWaitTicks++;
            if (leftReleased && ledPattern == 0x01) {
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
            updatePlayerScore(player1Score, 1);

            if (player1Score >= 3) {
                gameState = STATE_WIN;
                break;
            }

            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 0;  // Player 2 missed → Player 1 serves
            serve();
            gameState = STATE_SERVE;
            break;

        case STATE_LEFT_MISS:
            player2Score++;
            updatePlayerScore(player2Score, 2);

            if (player2Score >= 3) {
                gameState = STATE_WIN;
                break;
            }

            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 1;  // Player 1 missed → Player 2 serves
            serve();
            gameState = STATE_SERVE;
            break;

        case STATE_WIN:
            if (player1Score >= 3) {
                flashWinnerScore(1);
            } else if (player2Score >= 3) {
                flashWinnerScore(2);
            }

            player1Score = 0;
            player2Score = 0;
            updatePlayerScore(0, 1);
            updatePlayerScore(0, 2);
            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 1;
            serve();
            gameState = STATE_SERVE;
            break;
    }

    prevLeftBtn = currLeftBtn;
    prevRightBtn = currRightBtn;
}
}

/**
 * @brief Configures the SysTick timer for the game speed.
 * @param reloadValue The reload value determining the speed in ticks.
 */
void configureSysTick(uint32_t reloadValue)
{
    SysTick->LOAD  = reloadValue - 1;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief SysTick interrupt handler
 * Handles button debouncing and state machine for PLAY_MODE.
 */
void SysTick_Handler(void)
{
    msTimer++;

    // === Debounce Buttons ===
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

    // === PLAY_MODE State Machine ===
    if (led_mode == PLAY_MODE)
    {
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
                if (player1Score >= 3) {
                 gameState = STATE_WIN;
                  break;
                }    
                configureSysTick(currentSpeed);
                gameState = STATE_SHIFT_LEFT;
                break;

           case STATE_RIGHT_MISS:
            player1Score++;
            updatePlayerScore(player1Score, 1);
    
            if (player1Score >= 3) {  // 
          gameState = STATE_WIN;
         break;
    }

        currentSpeed = INITIAL_SPEED;
        configureSysTick(currentSpeed);
        currentServer = 0;  // Player 2 missed → Player 1 serves
       serve();
      gameState = STATE_SERVE;
        break;

    case STATE_LEFT_MISS:
    player2Score++;
    updatePlayerScore(player2Score, 2);

    if (player2Score >= 3) {  // 
        gameState = STATE_WIN;
        break;
    }

    currentSpeed = INITIAL_SPEED;
    configureSysTick(currentSpeed);
    currentServer = 1;  // Player 1 missed → Player 2 serves
    serve();
    gameState = STATE_SERVE;
    break;

            case STATE_WIN:
                if (player1Score >= 3) {
                    flashWinnerScore(1);
            } else if (player2Score >= 3) {
                    flashWinnerScore(2);
            }
                player1Score = 0;
                player2Score = 0;
                updatePlayerScore(0, 1);
               updatePlayerScore(0, 2);
            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 1; // or 0, depending on your preference
            serve();
             gameState = STATE_SERVE;
            break;

        }
    }
}

/**
 * @brief Handles logic for FLASH_LED_MODE
 * Only one LED is on at a time, and button presses shift it left or right.
 */
void handleFlashLedMode(void)
{
    static uint8_t prevLeftBtn = 1;
    static uint8_t prevRightBtn = 1;

    uint8_t currLeftBtn = buttons[BTN_LEFT].state;
    uint8_t currRightBtn = buttons[BTN_RIGHT].state;

    uint8_t currentPattern = getCurrentLedPattern();

    // === LEFT Button Released ===
    if (prevLeftBtn == 0 && currLeftBtn == 1)
    {
        if (currentPattern == 0x80)
            setLedPattern(0x01);
        else
            setLedPattern(currentPattern << 1);
    }

    // === RIGHT Button Released ===
    if (prevRightBtn == 0 && currRightBtn == 1)
    {
        if (currentPattern == 0x01)
            setLedPattern(0x80);
        else
            setLedPattern(currentPattern >> 1);
    }

    prevLeftBtn = currLeftBtn;
    prevRightBtn = currRightBtn;
}

