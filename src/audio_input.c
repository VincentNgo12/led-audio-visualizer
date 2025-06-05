#include "audio_input.h"
#include "led_driver.h"
#include "stm32f1xx.h"
#include "fft.h"
#include "stdint.h"

volatile uint16_t dma_buf[DMA_BUF_LEN] __attribute__((aligned(4)));// Store raw INMP441 SPI2 readings (use two 16-bit slots per INMP441's 24-bit data)
volatile uint16_t signal_buf[SIGNAL_BUF_LEN] __attribute__((aligned(4))); // Stores processed INMP441's signal
volatile bool signal_buf_half_ready = false; //adc_buf[] half Interupt Flag
volatile bool signal_buf_full_ready = false; //adc_buf[] full Interupt Flag


/*=====================================================================
***********************************************************************
    This function is used to initialized SPI2 for INMP441 along with
    DMA1 Channel 4 in order to capture and store signal with INMP441
***********************************************************************
======================================================================*/
void INMP441_Init(){
    /*====================================
       Code to initialize SPI2 (Mimics I2S) 
    ====================================*/
    // ===== Enable Clocks =====
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;     // Enable SPI2 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;     // Enable GPIOB clock
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;      // Enable DMA1 clock
    
    // ===== GPIO Configuration =====
    // PB13 - SPI2_SCK (input floating)
    GPIOB->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOB->CRH |= GPIO_CRH_CNF13_0; // Input floating

    // PB15 - SPI2_MOSI (I2S SD from INMP441)
    GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    GPIOB->CRH |= GPIO_CRH_CNF15_0; // Input floating

    // ===== SPI2 Configuration (slave mode) =====
    SPI2->CR1 &= ~SPI_CR1_MSTR;       // Slave mode
    SPI2->CR1 |= SPI_CR1_RXONLY;      // Recieve only mode
    SPI2->CR1 |= SPI_CR1_DFF;         // 16-bit data frame (read 2 words per sample)
    SPI2->CR1 &= ~SPI_CR1_CPOL;       // Clock polarity (CPOL = 0)
    SPI2->CR1 &= ~SPI_CR1_CPHA;       // Clock phase    (CPHA = 0)
    SPI2->CR1 &= ~SPI_CR1_SSM;        // Hardware NSS (not used here)

    SPI2->CR2 |= SPI_CR2_RXDMAEN;     // Enable RX DMA only


    /*====================================
        Code to initialize DMA1 Channel 4
        to transfer SPI2 serial data
        to dma_buf
    ====================================*/
    // ===== Configure DMA1 Channel 4 (SPI2_RX) =====
    DMA1_Channel4->CCR = 0;
    DMA1_Channel4->CPAR = (uint32_t)&SPI2->DR;    // Peripheral Address (SPI2 serial data)
    DMA1_Channel4->CMAR = (uint32_t)dma_buf;   // Memory Address (dma_buf array)
    DMA1_Channel4->CNDTR = DMA_BUF_LEN;        // dma_buf[] size
    
    DMA1_Channel4->CCR =
          DMA_CCR_MINC        // Memory increment mode
        | DMA_CCR_PSIZE_0     // Peripheral size 16-bit
        | DMA_CCR_MSIZE_0     // Memory size 16-bit 
        | DMA_CCR_CIRC        // Circular mode
        | DMA_CCR_HTIE        // Half transfer interrupt
        | DMA_CCR_TCIE        // Transfer complete interrupt
        | DMA_CCR_EN;         // Enable DMA

    DMA1->IFCR |= DMA_IFCR_CTCIF4 | DMA_IFCR_CHTIF4 | DMA_IFCR_CTEIF4; //Clear DMA flag before enable IRQ
    NVIC_EnableIRQ(DMA1_Channel4_IRQn); // Enable interrupt in NVIC

    TIMx_WS_SCK_Init(); // Initialize TIM1 for SCK and WS signals

    // ===== Enable SPI2 =====
    SPI2->CR1 |= SPI_CR1_SPE; // Enable SPI2
}


/*=====================================================================
***********************************************************************
    This function is used to initialized TIM2 Channel 1 (PA0) and
    TIM4 Channel 1 (PB6). TIM2 will generate SCK and TIM4 will
    generate WS for the INMP441 and SPI2 (slave mode)
***********************************************************************
======================================================================*/
void TIMx_WS_SCK_Init(){
    // 1. Enable peripheral clocks
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   // Enable TIM2
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;   // Enable TIM4
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA (for TIM2_CH1)
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;   // Enable GPIOB (for TIM4_CH1)

    // 2. Configure GPIOs for alternate function output push-pull
    // TIM2_CH1 on PA0
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |=  (GPIO_CRL_MODE0_1 | GPIO_CRL_MODE0_0); // Output 50 MHz
    GPIOA->CRL |=  (GPIO_CRL_CNF0_1);                     // AF Push-Pull

    // TIM4_CH1 on PB6
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
    GPIOB->CRL |=  (GPIO_CRL_MODE6_1 | GPIO_CRL_MODE6_0); // Output 50 MHz
    GPIOB->CRL |=  (GPIO_CRL_CNF6_1);                     // AF Push-Pull

    // 3. Disable timers before config
    TIM2->CR1 = 0;
    TIM4->CR1 = 0;

    // 4. TIM2: Setup for SCK 
    TIM2->PSC = 0;               // No prescaler
    TIM2->ARR = 25 - 1;              // Auto-reload 72MHz/25 = 2.88 MHz (SCK freq) 
    TIM2->CCR1 = TIM2->ARR/2;             // 50% duty
    TIM2->CCMR1 = (6 << 4);      // PWM mode 1 on CH1
    TIM2->CCER = TIM_CCER_CC1E;  // Enable CH1 output


    TIM2->CR2 |= TIM_CR2_MMS_1;      // MMS = 010: TRGO on Update event
    TIM4->SMCR = 0;
    TIM4->SMCR |= TIM_SMCR_TS_0;     // TS = ITR1 (trigger input from TIM2)


    // 5. TIM4: Drives WS 
    TIM4->PSC = 0;
    TIM4->ARR = 1500;       // 64 SCK cycles × 25 ticks/SCK = 1600 - 1
    TIM4->CCR1 = TIM4->ARR/2;       // 50% duty
    TIM4->CCMR1 = (6 << 4);      // PWM mode 1 on CH1
    TIM4->CCER = TIM_CCER_CC1E;  // Enable CH1 output
    
    // 6. Enable counters
    TIM2->CR1 |= TIM_CR1_CEN;
    //for (volatile int i = 0; i < 100; ++i) __asm volatile("nop"); // Offset
    TIM4->CR1 |= TIM_CR1_CEN;
}


// Post-process DMA buffer to get 24-bit audio data into signal_buf[]
void Process_DMA_Buffer(uint8_t half) {
    // Double dma_buf implementation: Process each half at a time.
    volatile uint16_t signal_index = (half == 0) ? 0 : (SIGNAL_BUF_LEN/2);
    volatile uint16_t dma_index = (half == 0) ? 0 : (DMA_BUF_LEN/2);

    for (int i = 0; i < (SIGNAL_BUF_LEN/2); i++) {
        uint16_t first16 = dma_buf[dma_index + 2*i]; // First SPI read (D23-D8)
        uint16_t second16 = dma_buf[dma_index + 2*i + 1]; // Second SPI read (D7-D0 + padding)
        int32_t sample24 = ((int32_t)(first16 << 8) | (second16 >> 8));
        if (sample24 & 0x800000) sample24 |= 0xFF000000; // sign-extend

        signal_buf[signal_index + i] = sample24 >> 8; // convert to int16_t
    }
}


/*=====================================================================
***********************************************************************
    This function is used to initialized ADC1 Channel 0 (PA0) along with
    DMA1 Channel 1 in order to capture and store signal with MAX4466
    module. Now I need to switch to using INMP441 but still I want to
    keep these code in case for later uses.
***********************************************************************
======================================================================*/
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
        to transfer ADC1_IN0 conversions
        to signal_buf
    ====================================*/
    // Enable DMA1 clock
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;  // Disable Channel 1 before config (CCR is Channel Config Register)

    // Configure DMA1 Channel1
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;         // Peripheral address (ADC1 conversions)
    DMA1_Channel1->CMAR = (uint32_t)signal_buf;           // Memory address (signal_buf array)
    DMA1_Channel1->CNDTR = SIGNAL_BUF_LEN;                // Number of data to transfer

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
}


//Process audio signal
void Signal_Buf_Process(uint8_t half){
    Process_DMA_Buffer(half); // Process raw dma_buf[] data

    // Double signal_buf implementation: Process each half at a time.
    volatile uint16_t* buf_ptr = (half == 0) ? &signal_buf[0] : &signal_buf[SIGNAL_BUF_LEN / 2];

    // Perform FFT on half of the adc_buf[] 
    FFT_Process(buf_ptr, USE_INMP441);

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
        signal_buf_half_ready = true; //Signal main loop
    }

    if (DMA1->ISR & DMA_ISR_TCIF1) {
        DMA1->IFCR |= DMA_IFCR_CTCIF1;  // Clear transfer complete flag
        signal_buf_full_ready = true; //Signal main loop
    }
}


/*====================================
    DMA1 Channel 4 Interupt Handler
  ====================================*/
void DMA1_Channel4_IRQHandler()
{
    if (DMA1->ISR & DMA_ISR_HTIF4) {
        DMA1->IFCR |= DMA_IFCR_CHTIF4;  // Clear half transfer flag
        signal_buf_half_ready = true; //Signal main loop
    }

    if (DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR |= DMA_IFCR_CTCIF4;  // Clear transfer complete flag
        signal_buf_full_ready = true; //Signal main loop
    }
}