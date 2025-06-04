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

    // ===== Configure PB13 (SCK), PB15 (MOSI as input), PB12 (WS/LRCK) =====
    // Set PB13 (SCK) as AF output push-pull, 50 MHz
    GPIOB->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOB->CRH |= (GPIO_CRH_MODE13_1 | GPIO_CRH_MODE13_0); // Output 50 MHz
    GPIOB->CRH |= GPIO_CRH_CNF13_1; // AF Push-Pull

    // Set PB15 (MOSI/SD) as input floating
    GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    GPIOB->CRH |= GPIO_CRH_CNF15_0; // Input floating

    // Set PB12 (WS/LRCK) as output push-pull (used for WS if manually toggled)
    GPIOB->CRH &= ~(GPIO_CRH_MODE12 | GPIO_CRH_CNF12);
    GPIOB->CRH |= GPIO_CRH_MODE12_1 | GPIO_CRH_MODE12_0; // Output 50 MHz
    GPIOB->CRH |= 0 << GPIO_CRH_CNF12_Pos; // General purpose push-pull

    // ===== SPI2 Configuration =====
    SPI2->CR1 = 0;                 // Disable SPI2 first
    SPI2->CR1 |= SPI_CR1_MSTR;     // Master mode
    SPI2->CR1 |= SPI_CR1_BR_2;     // Baud Rate = 72MHz / 32 = 2.25 MHz (for 2.25 MHz BCLK or 35.1 kHz × 2 channels × 32 bits)
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // Software NSS management
    SPI2->CR1 |= SPI_CR1_DFF;      // 16-bit data frame (Handle INMP441's 24-bit manually)
    SPI2->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN; // Enable DMA for RX and TX


    /*====================================
        Code to initialize DMA1 Channel 4
        to transfer SPI2 serial data
        to signal_buf
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


    /*============================================================
        Code to initialize DMA1 Channel 5, since SPI2 is not in
        RXONLY mode, we must transmit a dummy value to SPI2->DR
        everytime we receive data in order to drive the SPI2 SCK
        clock (so DMA1 Channel 4 can start transfering data).
    ==============================================================*/
    // ===== Configure DMA1 Channel 5 (SPI2_TX) =====
    static uint16_t dummy_word = 0x0000;

    DMA1_Channel5->CCR = 0;
    DMA1_Channel5->CPAR = (uint32_t)&SPI2->DR;       // Peripheral = SPI2 data register
    DMA1_Channel5->CMAR = (uint32_t)&dummy_word;     // Memory = dummy data (0x0000)
    DMA1_Channel5->CNDTR = DMA_BUF_LEN;              // Same size as RX buffer
    DMA1_Channel5->CCR =
    DMA_CCR_DIR          | // Memory-to-peripheral
    DMA_CCR_PSIZE_0      | // Peripheral size = 16-bit
    DMA_CCR_MSIZE_0      | // Memory size = 16-bit
    DMA_CCR_CIRC         | // Circular mode
    DMA_CCR_EN;            // Enable DMA



    /*====================================
        Code to initialize TIM2 to generate
        WS signal at 35.156 Khz
    ====================================*/
    // ===== Configure TIM_2_CH2 (PA1) =====
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   // Enable TIM2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA for PA1 output

    GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
    GPIOA->CRL |= GPIO_CRL_MODE1_1 | GPIO_CRL_MODE1_0; // Output mode, 50 MHz
    GPIOA->CRL |= GPIO_CRL_CNF1_1; // Alternate Function Push-Pull

    // --- Configure TIM2 for PWM at ~35.15625 kHz ---
    // Timer clock = 72 MHz
    // Want timer frequency = 35.15625 kHz
    // Choose: Prescaler = 0, ARR = 2047
    // 72 MHz / (2048) = 35156.25 Hz

    TIM2->PSC = 0;        // No prescaler
    TIM2->ARR = 2047;     // Auto-reload for 35.15625 kHz
    TIM2->CCR2 = 1024;    // 50% duty cycle (half of ARR)

    // --- Set PWM mode on Channel 2 ---
    TIM2->CCMR1 &= ~TIM_CCMR1_OC2M;
    TIM2->CCMR1 |= (6 << TIM_CCMR1_OC2M_Pos);  // PWM mode 1
    TIM2->CCMR1 |= TIM_CCMR1_OC2PE;            // Enable preload

    // --- Enable Channel 2 output ---
    TIM2->CCER |= TIM_CCER_CC2E;

    // --- Enable auto-reload preload and start timer ---
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->EGR |= TIM_EGR_UG;      // Generate update event
    TIM2->CR1 |= TIM_CR1_CEN;     // Start timer


    // ===== Enable SPI2 =====
    SPI2->CR1 |= SPI_CR1_SPE; // Enable SPI2
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