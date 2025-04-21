#include "led_setup.h"
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
volatile uint8_t currentServer = 1;  // 1 = Player 1, 0 = Player 2


// --- Global LED state ---
volatile uint8_t ledPattern = 0x01;
volatile uint8_t led_mode = PLAY_MODE;

void update_LEDs_PC5to12();


/*=============================================================================
 * init_LEDs_PC5to12()
 =============================================================================*/
void init_LEDs_PC5to12(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    for (int pin = 5; pin <= 12; pin++) {
        GPIOC->MODER   &= ~(3UL << (pin * 2));
        GPIOC->MODER   |=  (1UL << (pin * 2));
        GPIOC->OTYPER  &= ~(1UL << pin);
        GPIOC->OSPEEDR &= ~(3UL << (pin * 2));
        GPIOC->PUPDR   &= ~(3UL << (pin * 2));
    }

    // --- Player 1 Score: PC14, PC15, PH0 ---
    GPIOC->MODER &= ~(3UL << (14 * 2));
    GPIOC->MODER |=  (1UL << (14 * 2));
    GPIOC->OTYPER  &= ~(1UL << 14);
    GPIOC->OSPEEDR &= ~(3UL << (14 * 2));
    GPIOC->PUPDR   &= ~(3UL << (14 * 2));

    GPIOC->MODER &= ~(3UL << (15 * 2));
    GPIOC->MODER |=  (1UL << (15 * 2));
    GPIOC->OTYPER  &= ~(1UL << 15);
    GPIOC->OSPEEDR &= ~(3UL << (15 * 2));
    GPIOC->PUPDR   &= ~(3UL << (15 * 2));

    GPIOH->MODER &= ~(3UL << (0 * 2));
    GPIOH->MODER |=  (1UL << (0 * 2));
    GPIOH->OTYPER  &= ~(1UL << 0);
    GPIOH->OSPEEDR &= ~(3UL << (0 * 2));
    GPIOH->PUPDR   &= ~(3UL << (0 * 2));

    // --- Player 2 Score: PH1, PC2, PC3 ---
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
}

    void update_LEDs_PC5to12();

}

/*=============================================================================
 * update_LEDs_PC5to12()
 =============================================================================*/
void update_LEDs_PC5to12(void)
{
    GPIOC->ODR &= ~(0xFF << 5);
    GPIOC->ODR |= ((ledPattern & 0xFF) << 5);
}

int shiftRight(void)
{
    if (ledPattern == 0x01) return 0;
    ledPattern >>= 1;
    update_LEDs_PC5to12();
    return 1;
}

int shiftLeft(void)
{
    if (ledPattern == 0x80) return 0;
    ledPattern <<= 1;
    update_LEDs_PC5to12();
    return 1;
}

void serve(void)
{
    if (currentServer == 1) {
        ledPattern = 0x01;  // Start at PC5 (Player 1 serve)
    } else {
        ledPattern = 0x80;  // Start at PC12 (Player 2 serve)
    }
    update_LEDs_PC5to12();
} // Note: currentServer toggle should be handled in main after serve() if needed


