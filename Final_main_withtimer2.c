#include "stm32l476xx.h"
#include "led_setup.h"
#include "buttons.h"

/**
 ===================================================================
 * @file main.c
 * @brief 1D Pong game main file
 * @author Humza Rana & Mac
 * @Lab: Final Project
 * @Class: CPE 3000
 * -----------------------------------------------------
 *  In this lab, pins are enabled to light LEDs in two modes:
 *  PLAY_MODE and FLASH_LED_MODE.
 *  Two buttons are used to interact with the game and SysTick is
 *  used for regular timing.
 *  In PLAY_MODE, a pong game is emulated using a led array.
 *  The farthest left and right leds(blue and red) are the "paddles".
 *  The user button toggles between modes. 
 *   Debouncing is handled by Timer2 interupt.
 ===========================================================================
 */

// === Configuration ===
#define SYS_CLK_FREQ       4000000
#define MAX_SPEED_TICKS    100000 // Fastest game speed
#define SPEED_STEP         100000 // speed increment
#define INITIAL_SPEED      600000
#define FLASH_MODE_SPEED   20000  // ~5ms tick = 200Hz (4MHz / 20000)

// === Game States for PLAY_MODE ===
typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_RIGHT_HIT,
    STATE_RIGHT_MISS,
    STATE_LEFT_HIT,
    STATE_LEFT_MISS,
    STATE_WIN
} PongState;

// === Global Variables ===
static PongState gameState = STATE_SERVE;
static uint8_t player1Score = 0;
static uint8_t player2Score = 0;
uint32_t currentSpeed = INITIAL_SPEED;
uint32_t msTimer = 0;

// Function prototypes
void configureSysTick(uint32_t reloadValue);
void configureTimer(void);
void TIM2_IRQHandler(void);
void SysTick_Handler(void);
void handleFlashLedMode(void);

/******************************************
//main function
// Handles Initialization of pins and state machine.
// Also handles mode swtiching. 
*******************************************/
int main(void)
{
    // Initialize buttons and LEDs
    init_Buttons();
    init_LEDs_PC5to12();

    // Configure system timers
    configureSysTick(currentSpeed);  // Start SysTick for gameplay speed
    configureTimer();                // Timer2 handles button debouncing

    // Set initial serve state
    serve();

    // Set user LED (PA5) depending on initial mode
    if (led_mode == PLAY_MODE)
        GPIOA->ODR |= GPIO_ODR_OD5;   // Turn ON user LED for play mode
    else
        GPIOA->ODR &= ~GPIO_ODR_OD5;  // Turn OFF for flash mode

    // Variables to track user button state for rising edge detection
    uint8_t prevUserBtn = 1;
    uint8_t currUserBtn;

    while (1)
    {
        // Read current user button state (PC13)
        if ((GPIOC->IDR & (1 << 13)) != 0)
            currUserBtn = 1;
        else
            currUserBtn = 0;

        // Check for rising edge (button release)
        if (prevUserBtn == 0 && currUserBtn == 1)
        {
            // Toggle between PLAY_MODE and FLASH_LED_MODE
            if (led_mode == PLAY_MODE)
            {
                led_mode = FLASH_LED_MODE;

                // Indicate mode change by turning OFF user LED
                GPIOA->ODR &= ~GPIO_ODR_OD5;

                // Clear all playfield LEDs
                GPIOC->ODR &= ~(0xFF << 5);

                // Start flash mode from leftmost LED
                setLedPattern(0x01);

                // Set smooth, fast SysTick speed for flash mode
                configureSysTick(FLASH_MODE_SPEED);
            }
            else
            {
                led_mode = PLAY_MODE;

                // Turn ON user LED to indicate play mode
                GPIOA->ODR |= GPIO_ODR_OD5;

                // Restore normal game tick speed
                configureSysTick(currentSpeed);
            }
        }

        // Store current state for next comparison
        prevUserBtn = currUserBtn;

        // Handle LED shifting logic in FLASH_LED_MODE
        if (led_mode == FLASH_LED_MODE)
        {
            handleFlashLedMode();
        }
    }
}

/*****************************************************************************
 * configureSysTick()
 * @parameter: reloadValue - The reload value determining the speed ticks.
 * @return None
 * Configures the SysTick timer for the game speed.
 ******************************************************************************/
void configureSysTick(uint32_t reloadValue)
{
    SysTick->LOAD  = reloadValue - 1;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk;
}

/***********************************************************************
 * @ConfigureTimer()
 * @param None
 * @return None
 * Configures Timer 2 for button debouncing. 
* This config ensures that debouncing is happening faster then the Systick.
* Since Systick handles the game speed, the timer ensures the button reponse
* time is quick.
 **********************************************************************/
void configureTimer(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    TIM2->PSC = 2999;
    TIM2->ARR = 19;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 |= TIM_CR1_CEN;
    NVIC_EnableIRQ(TIM2_IRQn);
}

/********************************************************
 * TIM2_IRQHandler(void)
 * @param None
 * @return None
 * Timer 2 interrupt handles button debouncing. 
 * The decouncer 
 ******************************************************/
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;

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
    }
}

/***************************************************************
 * Systick_Handler()
 * @param None
 * @return None
 * SysTick interrupt handler for game state progression.
 *************************************************************/
void SysTick_Handler(void)
{
    msTimer++;

    // State machine logic
    if (led_mode == PLAY_MODE)
    {
        switch (gameState) // state machine
        {
        case STATE_SERVE: // begin serve
            serve(); // call serve function

            // Wait for current server to press their respective button
            if ((currentServer == 1 && buttons[BTN_LEFT].state == 0) ||
                (currentServer == 0 && buttons[BTN_RIGHT].state == 0)) 
            {
                if (ledPattern == 0x01) // If at player 1's paddle, shift left
                    gameState = STATE_SHIFT_LEFT;
                else if (ledPattern == 0x80) // if at player 2's paddle, shift right
                    gameState = STATE_SHIFT_RIGHT;
            }
            break;

        case STATE_SHIFT_LEFT:
            if (buttons[BTN_RIGHT].state == 0) // check if the right button was pressed
            {
                if (ledPattern == 0x80)
                    gameState = STATE_RIGHT_HIT; // if button is hit on paddle, it bounces back.
                else if (ledPattern == 0x40)
                    gameState = STATE_RIGHT_MISS; // if pressed early it's a miss
                // Other values are ignored
            }
            else if (!shiftLeft()) // if we can't shift further, it's a miss
            {
                gameState = STATE_RIGHT_MISS; // ball passed player 2
            }
            break;

        case STATE_SHIFT_RIGHT:
            if (buttons[BTN_LEFT].state == 0) // check if left button was pressed.
            {
                if (ledPattern == 0x01)
                    gameState = STATE_LEFT_HIT; // if button is hit on paddle, it bounces back.
                else if (ledPattern == 0x02)
                    gameState = STATE_LEFT_MISS; // if pressed early it's a miss
                // Other values are ignored
            }
            else if (!shiftRight()) // if we can't shift further, it's a miss
            {
                gameState = STATE_LEFT_MISS; // ball passed player 1
            }
            break;

        case STATE_RIGHT_HIT:
            if (currentSpeed > MAX_SPEED_TICKS + SPEED_STEP)
                currentSpeed -= SPEED_STEP; // make it faster
            configureSysTick(currentSpeed); // apply new speed
            gameState = STATE_SHIFT_RIGHT; // bounce back to player 1
            break;

        case STATE_LEFT_HIT:
            if (currentSpeed > MAX_SPEED_TICKS + SPEED_STEP)
                currentSpeed -= SPEED_STEP;
            configureSysTick(currentSpeed); // Increase the game speed
            gameState = STATE_SHIFT_LEFT; // bounce back to player 2
            break;

        case STATE_RIGHT_MISS:
            player1Score++; // player 1 gets a point
            updatePlayerScore(player1Score, 1);
            if (player1Score >= 3) {
                gameState = STATE_WIN; // check if player 1 wins
                break;
            }
            currentSpeed = INITIAL_SPEED; // reset speed
            configureSysTick(currentSpeed);
            currentServer = 0; // switch to player 2 serving
            serve(); // new serve
            gameState = STATE_SERVE;
            break;

        case STATE_LEFT_MISS:
            player2Score++; // player 2 gets a point
            updatePlayerScore(player2Score, 2);
            if (player2Score >= 3) {
                gameState = STATE_WIN; // check if player 2 wins
                break;
            }
            currentSpeed = INITIAL_SPEED; // reset speed
            configureSysTick(currentSpeed);
            currentServer = 1; // switch to player 1 serving
            serve(); // new serve
            gameState = STATE_SERVE;
            break;

        case STATE_WIN:
            if (player1Score >= 3)
                flashWinnerScore(1); // flash winning LEDs for player 1
            else if (player2Score >= 3)
                flashWinnerScore(2); // flash winning LEDs for player 2

            // Reset Pong game
            player1Score = 0;
            player2Score = 0;
            updatePlayerScore(0, 1);
            updatePlayerScore(0, 2);
            currentSpeed = INITIAL_SPEED;
            configureSysTick(currentSpeed);
            currentServer = 1;
            serve(); // return to beginning state
            gameState = STATE_SERVE;
            break;
        }
    }
}

/*****************************************************************************
 * handleFlashLedMode(void)
 * @param None
 * @return None
 * Only one LED is on at a time, and button presses shift it left or right.
 * A press and hold of the buttons are ensured.
 *****************************************************************************/
void handleFlashLedMode(void)
{
    static uint8_t prevLeftBtn = 1;
    static uint8_t prevRightBtn = 1;

    uint8_t currLeftBtn = buttons[BTN_LEFT].state;
    uint8_t currRightBtn = buttons[BTN_RIGHT].state; // ensure a button press and release happens
    
    uint8_t currentPattern = getCurrentLedPattern();
    // Check for rising edge (User button press)
    if (prevLeftBtn == 0 && currLeftBtn == 1)
    {
        if (currentPattern == 0x80)
            setLedPattern(0x01); // go the opposite edge
        else
            setLedPattern(currentPattern << 1); // shift by 1
    }

    if (prevRightBtn == 0 && currRightBtn == 1)
    {
        if (currentPattern == 0x01)
            setLedPattern(0x80); // go to opposite edge
        else
            setLedPattern(currentPattern >> 1); // shift by 1
    }

    prevLeftBtn = currLeftBtn;
    prevRightBtn = currRightBtn; // update the values for the buttons
}
