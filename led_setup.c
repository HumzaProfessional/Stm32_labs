#include "led_setup.h"
#include "stm32l476xx.h"

/************************************************************
 * @ file: led_setup.c
 *
 *This file is for setups the leds and buttons.
 *It also determined which led is on.
 *This is accomplished by clearing all the bits from pc8 - 15.
 *Then using logical OR with an global variable called pattern from main.c
 * Logical AND is used between the integer pattern and
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
 *  Configure PC8..PC15 as outputs (push-pull, low speed, no pull).
 *  defined constants aren't used since doing a for loop is more compact
===============================================================================
*/

void init_LEDs_PC8to15(void)
{
    // 1) Enable clock for GPIOC
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    // 2) Loop through pins 8..15
    for (int pin = 8; pin <= 15; pin++)
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
 *  @parameter: uint_t pattern - integer that determines what led is lit.
 *  @ return: none
 *
 * Write an 8-bit pattern to PC8..PC15 (bit0 => PC8 and bit7 => PC15).
 * Is responsible for the pattern.
 * Variable unint_8 pattern is a global variable accessed from main.c.
 ===========================================================================================
 */
void update_LEDs_PC8to15(uint8_t pattern)
{
    // Clear bits [8..15]
    GPIOC->ODR &= ~(0xFF << 8);

    // Insert the pattern into those 8 bits
    GPIOC->ODR |= ((pattern & 0xFF) << 8);

}
