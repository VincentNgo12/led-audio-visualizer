#include "led_driver.h"
#include "stm32f1xx.h"
#include "stdint.h"


//Code to initialize LED (Setup PWM with TIM3 and DMA)
void LED_Init(){
    /*===================================
    This section setup and configure TIM3
    =====================================*/

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; //Enable TIM3
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  //Enable PA clock (TIM3_CH1 uses PA6)
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6); //Clear PA6 config
    GPIOA->CRL |= (GPIO_CRL_CNF6_1 | GPIO_CRL_MODE6_1); // AF Push-Pull, Output 2MHz

    /*Configure TIM3 for PWD Output*/
    TIM3->PSC = TIM3_PRESCALER;  // = 0 (No prescaler)
    TIM3->ARR = TIM3_PERIOD;   // Auto-reload value (ARR = 100 - 1)
    TIM3->CCR1 = 50;                   // Initial duty cycle (50% since ARR is 100)
    TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; // PWM Mode (Output high while CNT < CCR1, low otherwise)
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;   // Enable preload
    TIM3->CCER  |= TIM_CCER_CC1E;     // Enable channel output (TIM3_CH1)
    TIM3->CR1   |= TIM_CR1_ARPE;      // Auto-reload preload enable


    /*===================================
    This section setup and configure DMA1
    =====================================*/
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN; //Enable DMA1
    static uint16_t pwm_data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 80, 70, 60, 50, 40, 30};  // Example PWM pattern
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;  // Disable Channel 3 before config (CCR is Channel Config Register)
    DMA1_Channel3->CPAR = (uint32_t)&TIM3->CCR1; //CPAR holds address of TIM3_CCR1 (PWD Duty Cycle)
    DMA1_Channel3->CMAR = (uint32_t)pwm_data; // DMA1 Channel 3 points to pwm_data[] array
    DMA1_Channel3->CNDTR = sizeof(pwm_data)/sizeof(pwm_data[0]); //Configure number of data element to transfer for channel

    /*Configure DMA1 Channel 3 Behaviour:
    -Direction Memory to Peripheral
    -Memory address increment
    -Circular Mode (restart from array begin when done)
    -Peripheral Data size = 16 bit
    -Memoruy Data size = 16 bit*/
    DMA1_Channel3->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0;

    TIM3->DIER |= TIM_DIER_UDE; // Enable DMA for TIM3 update events (Update PWD Duty via DMA when CNT reaches ARR)

    DMA1_Channel3->CCR |= DMA_CCR_EN; //Start DMA1 Channel 3
    TIM3->CR1 |= TIM_CR1_CEN; //Start TIM3_CH1

}