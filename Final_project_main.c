
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

    // Score LEDs for player 2 (PC2, PC3, PC15)
    GPIOC->MODER &= ~(GPIO_MODER_MODE15 | GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOC->MODER |=  (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE2_0 | GPIO_MODER_MODE3_0);
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT15 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED15 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD15 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);

    // User LED (PA5)
    GPIOA->MODER   = (GPIOA->MODER & ~GPIO_MODER_MODE5) | GPIO_MODER_MODE5_0;
    GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT5);
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5);
    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD5);

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
        ledPattern = 0x01;  // Start at PC5
        currentServer = 0;  // Next serve by Player 2
    } else {
        ledPattern = 0x80;  // Start at PC12
        currentServer = 1;  // Next serve by Player 1
    }
    update_LEDs_PC5to12();
}

