##include "led_setup.h"
#include "stm32l476xx.h"

/************************************************************
 * @file: led_setup.c
 *
 * This file sets up the LEDs and buttons.
 * It determines which LED is on, depending on the state of led_mode.
 * It clears all bits from PC6–PC13 and uses a global ledPattern variable
 * to determine what LED to light.
 ************************************************************/

#define PLAY_MODE 0
#define FLASH_LED_MODE 1

// --- Global game variables ---
volatile uint8_t ledPattern = 0x01; // Start with PC8 lit
volatile uint8_t led_mode = PLAY_MODE;

static uint8_t currentServer = 1;  // 1 = Player 1 (Blue), 0 = Player 2 (Red)
volatile uint8_t player1Score = 0;
volatile uint8_t player2Score = 0;
static uint8_t winner = 0;
static uint8_t ballServed = 0;

// --- Game state variable ---
typedef enum {
    STATE_SERVE,
    STATE_SHIFT_LEFT,
    STATE_SHIFT_RIGHT,
    STATE_CHECK_LEFT_HIT,
    STATE_CHECK_RIGHT_HIT,
    STATE_WIN
} GameState;

static GameState gameState = STATE_SERVE;

// --- Function Prototypes ---
void serve(void);
void playMode(void);
void score(int whoScored);
void displayPlayerScore(uint8_t score, uint8_t player);
void init_LEDs_PC5to12(void);
void update_LEDs_PC5to12(uint8_t ledPattern, uint8_t led_mode);
int moveRight(void);
int moveLeft(void);

/*=============================================================================
 * init_LEDs_PC6to13()
 * Configures PC6–PC13 as outputs, plus score LEDs for PA13–15 and PC2/3/15.
 =============================================================================*/
void init_LEDs_PC5to12(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    for (int pin = 5; pin <= 12; pin++) {

        GPIOC->MODER &= ~(3UL << (pin * 2));
        GPIOC->MODER |=  (1UL << (pin * 2));

        GPIOC->OTYPER  &= ~(1UL << pin);

        GPIOC->OSPEEDR &= ~(3UL << (pin * 2));

        GPIOC->PUPDR   &= ~(3UL << (pin * 2));
    }

    //------------------------------- Player 1 (Blue) Score LEDs: PA13–PA15 -------------------------------

    //------------------------------- Player 2 (Red) Score LEDs: PC2, PC3, PC15 -------------------------------
    GPIOC->MODER &= ~(GPIO_MODER_MODE15 | GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOC->MODER |=  (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE2_0 | GPIO_MODER_MODE3_0);
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT15 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED15 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD15 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);

    //------------------------------- User LED: PA5 -------------------------------
    GPIOA->MODER   = (GPIOA->MODER & ~GPIO_MODER_MODE5) | GPIO_MODER_MODE5_0;
    GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT5);
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5);
    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD5);
}

/*=========================================================================================
 * update_LEDs_PC6to13()
 * Displays the ledPattern on PC6–PC13 if led_mode == PLAY_MODE.
 ==========================================================================================*/
void update_LEDs_PC5to12(uint8_t ledPattern, uint8_t led_mode)
{
    GPIOC->ODR &= ~(0xFF << 5); // Clear PC6–PC13
    if (led_mode == PLAY_MODE) {
        GPIOC->ODR |= ((ledPattern & 0xFF) << 5);
    }
}

/*=========================================================================================
 * playMode()
 * The main game state machine for 1D Pong.
 ==========================================================================================*/
void playMode(void)
{
    switch (gameState) {
   case STATE_SERVE:
    serve();
    if (ledPattern == 0x01)
        gameState = STATE_SHIFT_RIGHT;
    else
        gameState = STATE_SHIFT_LEFT;
    break;


        case STATE_SHIFT_RIGHT:
            if (moveRight() == 0) {
                gameState = STATE_CHECK_RIGHT_HIT;
            }
            break;

        case STATE_SHIFT_LEFT:
            if (moveLeft() == 0) {
                gameState = STATE_CHECK_LEFT_HIT;
            }
            break;

        case STATE_CHECK_RIGHT_HIT:
            if ((GPIOC->IDR & (1 << 0)) == 0) {
                gameState = STATE_SHIFT_LEFT;
            } else {
                score(1); // Player 1 gets point
            }
            break;

        case STATE_CHECK_LEFT_HIT:
            if ((GPIOC->IDR & (1 << 1)) == 0) {
                gameState = STATE_SHIFT_RIGHT;
            } else {
                score(0); // Player 2 gets point
            }
            break;

        case STATE_WIN:
            for (int i = 0; i < 3; i++) {
                if (winner == 1) {
                    GPIOA->ODR |= (1 << 13) | (1 << 14) | (1 << 15);
                } else {
                    GPIOC->ODR |= (1 << 15) | (1 << 2) | (1 << 3);
                }
                for (volatile int d = 0; d < 500000; d++);
                if (winner == 1) {
                    GPIOA->ODR &= ~((1 << 13) | (1 << 14) | (1 << 15));
                } else {
                    GPIOC->ODR &= ~((1 << 15) | (1 << 2) | (1 << 3));
                }
                for (volatile int d = 0; d < 500000; d++);
            }

            player1Score = 0;
            player2Score = 0;
            winner = 0;
            currentServer = 1;
            displayPlayerScore(0, 1);
            displayPlayerScore(0, 2);
            gameState = STATE_SERVE;
            break;
    }
}

int moveRight(void)
{
    if (ledPattern == 0x80) return 0;
    ledPattern <<= 1;
    update_LEDs_PC5to12(ledPattern, led_mode);
    return 1;
}

int moveLeft(void)
{
    if (ledPattern == 0x01) return 0;
    ledPattern >>= 1;
    update_LEDs_PC5to12(ledPattern, led_mode);
    return 1;
}

void serve(void)
{
    if (currentServer == 1) {
        ledPattern = 0x01;
        currentServer = 0;
    } else {
        ledPattern = 0x80;
        currentServer = 1;
    }

    update_LEDs_PC5to12(ledPattern, led_mode);
    ballServed = 1;
}

void score(int whoScored)
{
    if (whoScored == 1) {
        player1Score++;
        displayPlayerScore(player1Score, 1);
        if (player1Score >= 3) {
            winner = 1;
            gameState = STATE_WIN;
        } else {
            gameState = STATE_SERVE;
        }
    } else {
        player2Score++;
        displayPlayerScore(player2Score, 2);
        if (player2Score >= 3) {
            winner = 2;
            gameState = STATE_WIN;
        } else {
            gameState = STATE_SERVE;
        }
    }
}

void displayPlayerScore(uint8_t score, uint8_t player)
{
    if (player == 1) {
        GPIOA->ODR &= ~((1 << 13) | (1 << 14) | (1 << 15));
        if (score >= 1) GPIOA->ODR |= (1 << 13);
        if (score >= 2) GPIOA->ODR |= (1 << 14);
        if (score >= 3) GPIOA->ODR |= (1 << 15);
    } else {
        GPIOC->ODR &= ~((1 << 15) | (1 << 2) | (1 << 3));
        if (score >= 1) GPIOC->ODR |= (1 << 15);
        if (score >= 2) GPIOC->ODR |= (1 << 2);
        if (score >= 3) GPIOC->ODR |= (1 << 3);
    }
}
