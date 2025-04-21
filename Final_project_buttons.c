include "led_setup.h"
#include "stm32l476xx.h"


/*=========================================================================================
 *  init_Buttons()
 *  @parameter: none
 *  @ return: none
 *
 * Initialize pc0 and pc1 as input for the buttons.
 ===========================================================================================
 */

//-------------------------------------------------------------------------------------
// Structure that encapsulates a debounced button
//-------------------------------------------------------------------------------------
typedef struct {
    uint32_t filter;         // 8-bit shift register for debouncing
    uint32_t state;          // 0 = pressed, 1 = released
    GPIO_TypeDef *port;      // Pointer to GPIO port
    uint32_t pin;            // Pin number (0–15)
} Button;

//-------------------------------------------------------------------------------------
// Button Indices and Definitions
//-------------------------------------------------------------------------------------
#define NUM_BUTTONS 3
#define BTN_RIGHT   0  // PC0
#define BTN_LEFT    1  // PC1
#define BTN_USER    2  // PC13

//-------------------------------------------------------------------------------------
// Exported global button array
//-------------------------------------------------------------------------------------
volatile Button buttons[NUM_BUTTONS] = {
    [BTN_RIGHT] = {0xFF, 1, GPIOC, 0},   // default released
    [BTN_LEFT]  = {0xFF, 1, GPIOC, 1},
    [BTN_USER]  = {0xFF, 1, GPIOC, 13}
};

//-------------------------------------------------------------------------------------
// Button Initialization
//-------------------------------------------------------------------------------------
void init_Buttons(void)
{
    // Enable GPIOC clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    // --- PC0 (BTN_RIGHT) ---
    GPIOC->MODER  &= ~GPIO_MODER_MODE0_Msk;   // Input mode
    GPIOC->PUPDR  &= ~GPIO_PUPDR_PUPD0_Msk;
    GPIOC->PUPDR  |=  GPIO_PUPDR_PUPD0_0;     // Pull-up

    // --- PC1 (BTN_LEFT) ---
    GPIOC->MODER  &= ~GPIO_MODER_MODE1_Msk;
    GPIOC->PUPDR  &= ~GPIO_PUPDR_PUPD1_Msk;
    GPIOC->PUPDR  |=  GPIO_PUPDR_PUPD1_0;

    // --- PC13 (BTN_USER) ---
    GPIOC->MODER  &= ~GPIO_MODER_MODE13_Msk;
    GPIOC->PUPDR  &= ~GPIO_PUPDR_PUPD13_Msk;
    GPIOC->PUPDR  |=  GPIO_PUPDR_PUPD13_0;     // Pull-up for stability
}
