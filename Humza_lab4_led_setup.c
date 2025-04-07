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


/*=========================================================================================
 *  init_Buttons()
 *  @parameter: none
 *  @ return: none
 *
 * Initialize pc0 and pc1 as input for the buttons.
 ===========================================================================================
 */
void init_Buttons(void)
{
    // 1) Enable clock for GPIOC
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    // PC0 => '00' input mode, '01' pull-up
    GPIOC->MODER  &= ~GPIO_MODER_MODE0_Msk;  // clears bits => input
    GPIOC->PUPDR  &= ~GPIO_PUPDR_PUPD0_Msk;
    GPIOC->PUPDR  |=  GPIO_PUPDR_PUPD0_0;    // pull-up

    // PC1 => '00' input mode, '01' pull-up
    GPIOC->MODER  &= ~GPIO_MODER_MODE1_Msk; // clear bits => input
    GPIOC->PUPDR  &= ~GPIO_PUPDR_PUPD1_Msk;
    GPIOC->PUPDR  |=  GPIO_PUPDR_PUPD1_0; // pull-up
}

/*=============================================================================
 * init_LEDs_PC8to15()
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
    for (int pin = 6; pin <= 13; pin++)
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
    // Clear bits [6..13]
    GPIOC->ODR &= ~(0xFF << 6);

    if (led_mode == SINGLE_LED_MODE) {
        // Write the given pattern to PC6..PC13.
        GPIOC->ODR |= ((ledPattern & 0xFF) << 6);


    }
    else if (led_mode == FLASH_LED_MODE) {
        // For flashing mode, use 'pattern' as a simple toggle indicator.
        // For example, if 'pattern' is even, light the lower four LEDs; if odd, light the upper four LEDs.
        if ((ledPattern & 0x01) == 0)
            GPIOC->ODR |= (0x0F << 6); // lower 4 LEDs on (PC8-11)
        else
            GPIOC->ODR |= (0xF0 << 6); // upper 4 LEDs on (PC12-15)
    }
}
