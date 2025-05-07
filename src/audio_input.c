#include "audio_input.h"
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

    // Calibrate ADC
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL);

    // Power on ADC
    ADC1->CR2 |= ADC_CR2_ADON; //Enable ADC
    for (volatile int i = 0; i < 1000; i++);  // Short delay
    ADC1->CR2 |= ADC_CR2_ADON; //Start calibration (second write)

    // Configure ADC
    ADC1->SMPR2 |= ADC_SMPR2_SMP0;  // Max sampling time for Ch0 (71.5 cycles)
    ADC1->SQR1 = 0;                 // 1 conversion in sequence
    ADC1->SQR3 = 0;                 // Convert Channel 0 (PA0)   

    ADC1->CR2 |= ADC_CR2_CONT;      // Continuous mode
    ADC1->CR2 |= ADC_CR2_DMA;       // Request DMA on each conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;   // Start conversion




    /*====================================
        Code to initialize DMA1 Channel 1
        to transfer ADC0_IN0 conversions
        to adc_buf
    ====================================*/
    // Enable DMA1 clock
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

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

    // Enable interrupt in NVIC
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
    }
}


