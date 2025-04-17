#include "led_setup.h"
#include "stm32l476xx.h"

/************************************************************
 * @ file: led_setup.c
 *
 *This file is for setups the leds and buttons.
 *It also determined which led is on, depending on the state of led_mode integer.
 *This is accomplished by clearing all the bits from pc6 - 13.
 *Then using logical OR with an global variable called pattern from main.c
 *
 **************************************************
 */


// Modes for LED output
#define PLAY_MODE 0
#define FLASH_LED_MODE  1

volatile uint8_t ledPattern = 0x01; // start PC8 lit
 volatile uint8_t led_mode = PLAY_MODE;

 static uint8_t currentServer = 1;  // 1 = Blue Player 1, 0 = Red Player
 volatile uint8_t player1Score = 0;
 volatile uint8_t player2Score = 0;
static uint8_t winner = 0;

 // state machine states of PLAY_MODE
 typedef enum {
     STATE_SERVE,
     STATE_SHIFT_LEFT,
     STATE_SHIFT_RIGHT,
     STATE_CHECK_LEFT_HIT,
     STATE_CHECK_RIGHT_HIT,
	 STATE_WIN
 } GameState;

/*=============================================================================
 * init_LEDs()
 *
 * @param: none
 * @return: none
 *  Configure PC6..PC13 as outputs (push-pull, low speed, no pull).
 *  Defined constants aren't used since doing a for loop is more compact.
===============================================================================
*/

void init_LEDs_PC6to13(void)
{
    // 1) Enable clock for GPIOC
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    // 2) Loop through pins 6..13
    // Leds for the pong playfield.
    //
    for (int pin = 5; pin <= 12; pin++)
    {
        // Output mode => '01' in MODER
        GPIOC->MODER   &= ~(3UL << (pin * 2));
        GPIOC->MODER   |=  (1UL << (pin * 2));

        // Push-pull => OTYPER = 0
        GPIOC->OTYPER  &= ~(1UL << pin);

        // Low speed => OSPEEDR = 00
        GPIOC->OSPEEDR &= ~(3UL << (pin * 2));

        // No pull => PUPDR = 00
        GPIOC->PUPDR   &= ~(3UL << (pin * 2));
    }

    //-------------------------------blue: player 1 score----------------------------------
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE13 | GPIO_MODER_MODE14 | GPIO_MODER_MODE15);
    GPIOA->MODER |=  (GPIO_MODER_MODE13_0 | GPIO_MODER_MODE14_0 | GPIO_MODER_MODE15_0);

    GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT13 | GPIO_OTYPER_OT14 | GPIO_OTYPER_OT15);

    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED13 | GPIO_OSPEEDR_OSPEED14 | GPIO_OSPEEDR_OSPEED15);

    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD13 | GPIO_PUPDR_PUPD14 | GPIO_PUPDR_PUPD15);


    //--------------------------------red: player 2 score--------------------------------
    // Clear mode bits for PC15, PC2, PC3 (set as output)
    GPIOC->MODER &= ~(GPIO_MODER_MODE15 | GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOC->MODER |=  (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE2_0 | GPIO_MODER_MODE3_0);

    //  Set output type to push-pull
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT15 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);

    //  Set to low speed
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED15 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);

    //  No pull-up/pull-down
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD15 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);


    //-------------------------------user led -----------------------------------
      RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // enable clock signal for port

      GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE5) | GPIO_MODER_MODE5_0;

      GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT5);   // push-pull (PA5=0)
      GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5);  // low speed (PA5=00)
      GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD5);  // no pullup,pulldown (PA5=00)
}


/*=========================================================================================
 *  @parameter: uint8_t ledPattern - integer that determines what led is lit.
 *              uint8_t led_mode - integer that determine whether the leds are in FLASH_MODE or SINGLE_MODE
 *  @ return: none
 *
 * Write an 8-bit pattern to PC6..PC13 (bit0 => PC6 and bit7 => PC13).
 * Is responsible for the pattern
 * Variable unint_8 pattern is a global variable accessed from main.c.
 * So is led_mode which determines how the led range should behave.
 ===========================================================================================
 */
void update_LEDs_PC6to13(uint8_t ledPattern, uint8_t led_mode)
{
    // Clear bits [5..12]
    GPIOC->ODR &= ~(0xFF << 5);

    if (led_mode == SINGLE_LED_MODE) {
        // Write the given pattern to PC6..PC13.
        GPIOC->ODR |= ((ledPattern & 0xFF) << 5);

    }

}

void playMode(void)
{
    switch (GameState) {

        case STATE_SERVE:
            serve();
            ballServed = 0;
            if (ballServed) {
                if (ledPattern == 0x01)
                    gameState = STATE_SHIFT_RIGHT;
                else
                    gameState = STATE_SHIFT_LEFT;
            }
            break;

        case STATE_SHIFT_RIGHT:
            if (moveRight() == 0) {
                // Ball reached far right, now check for hit
                gameState = STATE_CHECK_RIGHT_HIT;
            }
            break;

        case STATE_SHIFT_LEFT:
            if (moveLeft() == 0) {
                // Ball reached far left, now check for hit
                gameState = STATE_CHECK_LEFT_HIT;
            }
            break;

        case STATE_CHECK_RIGHT_HIT:
            if ((GPIOC->IDR & (1 << 0)) == 0) { // PC0 = right player hit
                gameState = STATE_SHIFT_LEFT;
            } else {
                score(1); // Player 1 scores
                gameState = STATE_SERVE;
            }
            break;

        case STATE_CHECK_LEFT_HIT:
            if ((GPIOC->IDR & (1 << 1)) == 0) { // PC1 = left player hit
                gameState = STATE_SHIFT_RIGHT;
            } else {
                score(0); // Player 2 scores
                gameState = STATE_SERVE;
            }
            break;
    }
}

int moveRight(void)
{
    if (ledPattern == 0x80) return 0; // Missed: far right
    ledPattern <<= 1;
    update_LEDs(ledPattern);
    return 1;
}

int moveLeft(void)
{
    if (ledPattern == 0x01) return 0; // Missed: far left
    ledPattern >>= 1;
    update_LEDs(ledPattern);
    return 1;
}

void serve(void)
{
    if (currentServer == 1) {
        ledPattern = 0x01;  // Start at PC8
        currentServer = 0;  // Next serve from player 2
    } else {
        ledPattern = 0x80;  // Start at PC15
        currentServer = 1;  // Next serve from player 1
    }

    update_LEDs(ledPattern);
    ballServed = 1;
}

void score(int whoScored)  // 1 = Player 1, 0 = Player 2
{
    if (whoScored == 1) {
        player1Score++;
        displayPlayerScore(player1Score, 1);
        if (player1Score >= 3) {
            winner = 1;
            gameState = STATE_WIN;
        }
    } else {
        player2Score++;
        displayPlayerScore(player2Score, 2);
        if (player2Score >= 3) {
            winner = 2;
            gameState = STATE_WIN;
        }
    }
}


void displayPlayerScore(uint8_t score, uint8_t player)
{
    if (player == 1) {
        // Clear PA13–PA15
        GPIOA->ODR &= ~((1 << 13) | (1 << 14) | (1 << 15));

        if (score >= 1) GPIOA->ODR |= (1 << 13);
        if (score >= 2) GPIOA->ODR |= (1 << 14);
        if (score >= 3) GPIOA->ODR |= (1 << 15);
    } else {
        // Clear PC15, PC2, PC3
        GPIOC->ODR &= ~((1 << 15) | (1 << 2) | (1 << 3));

        if (score >= 1) GPIOC->ODR |= (1 << 15);
        if (score >= 2) GPIOC->ODR |= (1 << 2);
        if (score >= 3) GPIOC->ODR |= (1 << 3);
    }
}
