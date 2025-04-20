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

// Structure that encapsulates a debounced button
typedef struct {
    uint32_t filter;         // Recent input history, shifted in every tick
    uint32_t state;          // Current debounced state (0 = pressed, 1 = released)
    GPIO_TypeDef *port;      // GPIO port (e.g., GPIOC)
    uint32_t pin;            // GPIO pin number (e.g., 13)
} Button;


#define NUM_BUTTONS 3
#define BTN_RIGHT   0
#define BTN_LEFT    1
#define BTN_USER    2

volatile Button buttons[NUM_BUTTONS] = {
    [BTN_RIGHT] = {0, 1, GPIOC, 0},   // PC0
    [BTN_LEFT]  = {0, 1, GPIOC, 1},   // PC1
    [BTN_USER]  = {0, 1, GPIOC, 13}   // PC13 (user button)
};


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

    //----------------------Configure PC 13 as input (user button)---------------------
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;    // Enable port C clock

   	  GPIOC->MODER   &= ~(GPIO_MODER_MODE13);              // input pin (PC13=00)
   	  GPIOC->PUPDR   &= ~(GPIO_PUPDR_PUPD13);           // no pullup,pulldown (PC13=00)

}


