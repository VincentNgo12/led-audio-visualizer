#include "audio_input.h"
#include "led_driver.h"
#include "stm32f1xx.h"
#include "stdint.h"

volatile uint16_t adc_buf[ADC_BUF_LEN] __attribute__((aligned(4)));

void ADC1_Init(){
    /*====================================
       Code to initialize ADC1_IN0 (AP0)
    ====================================*/

    // Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN; //Enable Port A and ADC1 Clock

    // Set PA0 to analog mode
    GPIOA->CRL &= ~GPIO_CRL_MODE0; //(00 - input mode)
    GPIOA->CRL &= ~GPIO_CRL_CNF0; //(00 - analog mode)

    // Set ADC prescaler:(must be ≤ 14 MHz)
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; //(72MHz / 6 = 12MHz)

    // Power on ADC
    ADC1->CR2 |= ADC_CR2_ADON;  // Power on ADC
    //for (volatile uint32_t i = 0; i < 1000; i++);  // Small Delay (1ms)

    // Calibrate ADC
    ADC1->CR2 |= ADC_CR2_CAL;       // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL); // Wait

    // Configure ADC
    ADC1->SMPR2 |= ADC_SMPR2_SMP0;  // Max sampling time for Ch0 (71.5 cycles)
    ADC1->SQR1 = 0;                 // 1 conversion in sequence
    ADC1->SQR3 = 0;                 // Convert Channel 0 (PA0)   

    ADC1->CR2 |= ADC_CR2_CONT;      // Continuous mode
    ADC1->CR2 |= ADC_CR2_DMA;       // Request DMA on each conversion

    // Important trick: toggle ADON again before starting
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;   // Start conversion




    /*====================================
        Code to initialize DMA1 Channel 1
        to transfer ADC0_IN0 conversions
        to adc_buf
    ====================================*/
    // Enable DMA1 clock
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;  // Disable Channel 1 before config (CCR is Channel Config Register)

    // Configure DMA1 Channel1
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;         // Peripheral address (ADC1 conversions)
    DMA1_Channel1->CMAR = (uint32_t)adc_buf;           // Memory address (adc_buf array)
    DMA1_Channel1->CNDTR = ADC_BUF_LEN;                // Number of data to transfer

    DMA1_Channel1->CCR =
          DMA_CCR_MINC       // Memory increment mode
        | DMA_CCR_PSIZE_0    // Peripheral size: 16 bits
        | DMA_CCR_MSIZE_0    // Memory size: 16 bits
        | DMA_CCR_CIRC       // Circular mode
        | DMA_CCR_TCIE       // Transfer complete interrupt enable
        | DMA_CCR_EN;        // Enable DMA channel

    DMA1_Channel1->CCR &= ~DMA_CCR_DIR;  // DIR=0: Peripheral→Memory (ADC1->DR → adc_buf)

    DMA1_Channel1->CCR |= DMA_CCR_EN; //Start DMA1 Channel 1    

    // Enable interrupt in NVIC
    DMA1->IFCR |= DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1; //Clear DMA flag before enable IRQ
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}


//Get ADC1 Input
uint16_t ADC1_Read()
{
    while (!(ADC1->SR & ADC_SR_EOC)); // Wait for conversion complete
    return ADC1->DR;
}



/*====================================
    DMA1 Interupt Handler
  ====================================*/
void DMA1_Channel1_IRQHandler()
{
    if (DMA1->ISR & DMA_ISR_TCIF1) {
        DMA1->IFCR |= DMA_IFCR_CTCIF1;  // Clear interrupt flag

        // Optionally process data in adc_buf here
        uint32_t sum = 0;
        for (int i = 0; i < ADC_BUF_LEN; i++) {
            sum += adc_buf[i];
        }
        uint16_t avg = sum / ADC_BUF_LEN;

        int32_t diff = avg - ADC_OFFSET;
        if (diff < 0) diff = 0;

        TIM3->CCR1 = (uint16_t)diff;  // Set PWM duty cycle
    }
}


