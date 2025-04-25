#include "led_setup.h"
#include "stm32l476xx.h"

/*=================================================================
 * @file: led_setup.c
 * @brief: LED and score setup functions for 1D Pong
 *
 * This file contains functions to initialize and control LEDs
 * used for the playfield and scoring in a 1D Pong game.
 * It configures PC5–PC12 for the playfield, and uses PC14, PC15,
 * PH0 (Player 1 score) and PH1, PC2, PC3 (Player 2 score).
 * Functions also include LED shifting logic and serving logic.
 *===============================================================*/

#define PLAY_MODE 0
#define FLASH_LED_MODE 1

volatile uint8_t ledPattern = 0x01;
volatile uint8_t led_mode = PLAY_MODE;
volatile uint8_t currentServer = 1;  // 1 = Player 1, 0 = Player 2

/***************************************************************************
 * init_LEDs_PC5to12()
 * @paramters: None
 *  @return: None
 * Configures the GPIO pins for the 8 playfield LEDs (PC5–PC12)
 * and the score LEDs for both players.
 ***************************************************************************/
void init_LEDs_PC5to12(void)
{   //------------Enable all clocks----------------
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOHEN;

    // --- Playfield LEDs: PC5–PC12 ---
    for (int pin = 5; pin <= 12; pin++) {
   GPIOC->MODER   &= ~(3UL << (pin * 2));
   GPIOC->MODER   |=  (1UL << (pin * 2));
   GPIOC->OTYPER  &= ~(1UL << pin);
   GPIOC->OSPEEDR &= ~(3UL << (pin * 2));
    GPIOC->PUPDR   &= ~(3UL << (pin * 2));
    }

    // --- Player 1(Blue) Score: PC14, PC15, PH0 ---
    GPIOB->MODER &= ~(3UL << (8 * 2));
    GPIOB->MODER |=  (1UL << (8 * 2));
    GPIOB->OTYPER &= ~(1UL << 8);
    GPIOB->OSPEEDR &= ~(3UL << (8 * 2));
    GPIOB->PUPDR &= ~(3UL << (8 * 2));

    GPIOB->MODER &= ~(3UL << (9 * 2));
    GPIOB->MODER |=  (1UL << (9 * 2));
    GPIOB->OTYPER &= ~(1UL << 9);
    GPIOB->OSPEEDR &= ~(3UL << (9 * 2));
    GPIOB->PUPDR &= ~(3UL << (9 * 2));

    GPIOH->MODER &= ~(3UL << (0 * 2));
    GPIOH->MODER |=  (1UL << (0 * 2));
    GPIOH->OTYPER  &= ~(1UL << 0);
    GPIOH->OSPEEDR &= ~(3UL << (0 * 2));
    GPIOH->PUPDR   &= ~(3UL << (0 * 2));

    // --- Player 2(Red) Score: PH1, PC2, PC3 ---
    GPIOH->MODER &= ~(3UL << (1 * 2));
    GPIOH->MODER |=  (1UL << (1 * 2));
    GPIOH->OTYPER  &= ~(1UL << 1);
    GPIOH->OSPEEDR &= ~(3UL << (1 * 2));
    GPIOH->PUPDR   &= ~(3UL << (1 * 2));

    GPIOC->MODER &= ~(3UL << (2 * 2));
    GPIOC->MODER |=  (1UL << (2 * 2));
    GPIOC->OTYPER  &= ~(1UL << 2);
    GPIOC->OSPEEDR &= ~(3UL << (2 * 2));
    GPIOC->PUPDR   &= ~(3UL << (2 * 2));

    GPIOC->MODER &= ~(3UL << (3 * 2));
    GPIOC->MODER |=  (1UL << (3 * 2));
    GPIOC->OTYPER  &= ~(1UL << 3);
    GPIOC->OSPEEDR &= ~(3UL << (3 * 2));
    GPIOC->PUPDR   &= ~(3UL << (3 * 2));

    // ----Configure user LED (LD2 -> Port A, bit 5)-------------------------
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE5) | GPIO_MODER_MODE5_0; // output pin (PA5=01)
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT5); // push-pull (PA5=0)
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5); // low speed (PA5=00)
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5); // 
}

/****************************************************************************
 * update_LEDs_PC5to12()
 *  @paramter: None
 * @return: None
 * Updates the playfield LEDs using the current ledPattern.
****************************************************************************/
void update_LEDs_PC5to12(void)
{
    GPIOC->ODR &= ~(0xFF << 5);  // Clear PC5–PC12
    GPIOC->ODR |= ((ledPattern & 0xFF) << 5);  // Set new pattern
}

/****************************************************************************
 * shiftRight()
 * @parameter: None
 * @return: 1 if the shift was occurs all the way.
 *          0 if already at the rightmost led and no shift occured.
 * Shifts the ball one LED to the right. Returns 0 if at end.
 ****************************************************************************/
int shiftRight(void)
{
    if (ledPattern == 0x01) return 0;
    ledPattern >>= 1;
    update_LEDs_PC5to12();
    return 1;
}

/****************************************************************************
 * shiftLeft()
   @parameter: None
   @return: 1 if the shift was occurs all the way.
            0 if already at the leftmost led and no shift occured.
 * Shifts the ball one LED to the left. Returns 0 if at end.
****************************************************************************/
int shiftLeft(void)
{
    if (ledPattern == 0x80) return 0;
    ledPattern <<= 1;
    update_LEDs_PC5to12();
    return 1;
}

/****************************************************************************
 * serve()
 * @parameter: None
 * @parameter: None
 * Places the LED ball at the starting position based on the server.
 ****************************************************************************/
void serve(void)
{
    if (currentServer == 1) {
        ledPattern = 0x01;  // Player 1 serve from left
    } else {
        ledPattern = 0x80;  // Player 2 serve from right
    }
    update_LEDs_PC5to12(); // make the pattern appear
}

/****************************************************************************
 * updatePlayerScore()
 * @parameter: uint8_t score, uint8_t player
 * @return: None
 * Flash LEDs for the score of the player that won 3 matches.
 ****************************************************************************/
void updatePlayerScore(uint8_t score, uint8_t player)
{
    if (player == 1) {
        // Player 1 Score LEDs: PB8, PB9, PH0
        GPIOB->ODR &= ~((1 << 8) | (1 << 9));
        GPIOH->ODR &= ~(1 << 0);

        if (score >= 1) GPIOB->ODR |= (1 << 8);
        if (score >= 2) GPIOB->ODR |= (1 << 9);
        if (score >= 3) GPIOH->ODR |= (1 << 0);
    }
    else if (player == 2) {
        // Player 2: PH1, PC2, PC3
        GPIOH->ODR &= ~(1 << 1);
        GPIOC->ODR &= ~((1 << 2) | (1 << 3));

        if (score >= 1) GPIOH->ODR |= (1 << 1);
        if (score >= 2) GPIOC->ODR |= (1 << 2);
        if (score >= 3) GPIOC->ODR |= (1 << 3);
    }
}

/***************************************************************************
 * flashWinnerScore()
 * @Parameter: uint8_t winner - Internal variable that determines what player leds should toggle.
 * @return: None
 * Lights up LEDs for the score of the specified player (1 or 2).
 ***************************************************************************/
void flashWinnerScore(uint8_t winner)
{
    for (int i = 0; i < 18; i++)  // Flash 9 times
    {
        if (winner == 1)
        {
            GPIOB->ODR ^= (1 << 8) | (1 << 9);  // Toggle PB8 & PB9
            GPIOH->ODR ^= (1 << 0);             // Toggle PH0
        }
        else if (winner == 2)
        {
            GPIOH->ODR ^= (1 << 1);             // Toggle PH1
            GPIOC->ODR ^= (1 << 2) | (1 << 3);   // Toggle PC2 & PC3
        }

        for (volatile int d = 0; d < 50000; d++);  // Delay
    }
}

/***************************************************
 * getCurrentLedPattern
 * @param None
 * @return The current 8-bit LED pattern stored in ledPattern
 ************************************************************/
uint8_t getCurrentLedPattern(void) {
    return ledPattern;
}

void setLedPattern(uint8_t pattern) {
    ledPattern = pattern;
    update_LEDs_PC5to12();
}
