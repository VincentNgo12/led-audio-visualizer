#include <stdint.h>
#include "stm32f1xx.h"
#include "led_driver.h"
#include "audio_input.h"

#define LED_PIN 13

//Prototypes
void SysTick_Init(void);
void delay_ms(uint32_t ms);

int main(void)
{
    // Enable GPIOC clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Configure PC13 as General Purpose Output Push-Pull
    GPIOC->CRH &= ~(0xF << ((LED_PIN - 8) * 4));  // Clear config bits
    GPIOC->CRH |=  (0x1 << ((LED_PIN - 8) * 4));  // Set MODE13 to Output 10MHz

    SysTick_Init(); //Initialize SysTick
    FFT_Init(); // Initialize FFT
    ADC1_Init();
    LED_Init();

    while (1)
    {
        //GPIOC->ODR ^= (1 << LED_PIN);  // Toggle PC13
        //delay_ms(1000);

        //Process Signal Data if interupt flags are raised
        if (signal_buf_half_ready && pwm_ready) {
            signal_buf_half_ready = false;
            pwm_ready = false;
            Signal_Buf_Process(0);  // First half
        }

        if (signal_buf_full_ready && pwm_ready) {
            signal_buf_full_ready = false;
            pwm_ready = false;
            Signal_Buf_Process(1);  // Second half
        }
    }
}


void SysTick_Init(void)
{
    // The system clock is set to 72MHZ in system_stm32f1xx.c
    SysTick->LOAD = 72000 - 1;
    SysTick->VAL = 0;                     // Clear current value register
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;  // Enable SysTick, processor clock
}

void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++)
    {
        // Wait until the COUNTFLAG is set
        while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    }
}