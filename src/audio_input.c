#include "audio_input.h"
#include "led_driver.h"
#include "stm32f1xx.h"
#include "fft.h"
#include "stdint.h"

volatile uint16_t adc_buf[ADC_BUF_LEN] __attribute__((aligned(4)));
volatile bool adc_buf_half_ready = false; //adc_buf[] half Interupt Flag
volatile bool adc_buf_full_ready = false; //adc_buf[] full Interupt Flag

void ADC1_Init(){
    /*====================================
        We will use TIM2 to trigger ADC1
        capture at 44.1 kHz sampling rate.
        Code to initialize TIM2
    ======================================*/
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock
    TIM2->PSC = 71;      // 72MHz / (71+1) = 1 MHz
    TIM2->ARR = 22;      // 1 MHz / (22+1) ≈ 43.47 kHz (~close to 44.1kHz)

    // Set update event as TRGO (trigger output)
    TIM2->CR2 &= ~TIM_CR2_MMS;        // Clear MMS bits
    TIM2->CR2 |= TIM_CR2_MMS_1;       // MMS = 010: Update event as trigger output (TRGO)

    // Start TIM2
    TIM2->CR1 |= TIM_CR1_CEN;



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

    // Calibrate ADC
    ADC1->CR2 |= ADC_CR2_CAL;       // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL); // Wait

    // Configure ADC
    ADC1->SMPR2 |= ADC_SMPR2_SMP0;  // Max sampling time for Ch0 (71.5 cycles) (~142,857 samples/sec)
    ADC1->SQR1 = 0;                 // 1 conversion in sequence
    ADC1->SQR3 = 0;                 // Convert Channel 0 (PA0)   
    // Set external trigger to TIM2 TRGO (EXTSEL = 011)
    ADC1->CR2 &= ~ADC_CR2_EXTSEL;
    ADC1->CR2 |= (3 << ADC_CR2_EXTSEL_Pos); // EXTSEL = 011: TIM2 TRGO
    ADC1->CR2 |= ADC_CR2_EXTTRIG; // Enable external trigger

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
    //These two interupts below are used to implement double buffer for adc_buf[]
    DMA1_Channel1->CCR |= DMA_CCR_TCIE;  // Transfer Complete Interrupt
    DMA1_Channel1->CCR |= DMA_CCR_HTIE;  // Half Transfer Interrupt
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);


    // Initialize FFT
    FFT_Init();
}

/*=======================
  ADC1 Input Processings
========================*/
//Get ADC1 Input
uint16_t ADC1_Read()
{
    while (!(ADC1->SR & ADC_SR_EOC)); // Wait for conversion complete
    return ADC1->DR;
}


//Process ADC1 Input
void ADC_Buf_Process(uint8_t half){
    // Double adc_buf implementation: Process each half at a time.
    volatile uint16_t* buf_ptr = (half == 0) ? &adc_buf[0] : &adc_buf[ADC_BUF_LEN / 2];

    // Perform FFT on half of the adc_buf[] 
    FFT_Process(buf_ptr);

    Update_Led_Colors(); // Update LED strip color data (led_colors[])
    Encode_Led_Data(); // Apply LED strip update
}


/*====================================
    DMA1 Channel 1 Interupt Handler
  ====================================*/
void DMA1_Channel1_IRQHandler()
{
    if (DMA1->ISR & DMA_ISR_HTIF1) {
        DMA1->IFCR |= DMA_IFCR_CHTIF1;  // Clear half transfer flag
        adc_buf_half_ready = true; //Signal main loop
    }

    if (DMA1->ISR & DMA_ISR_TCIF1) {
        DMA1->IFCR |= DMA_IFCR_CTCIF1;  // Clear transfer complete flag
        adc_buf_full_ready = true; //Signal main loop
    }
}

