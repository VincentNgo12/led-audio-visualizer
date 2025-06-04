#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include "fft.h"
#include <stdint.h>
#include <stdbool.h>

// Function prototypes
void ADC1_Init();
void TIM1_WS_SCK_Init();
void INMP441_Init();
void DMA_Buf_Process(uint8_t half); // Post-process DMA buffer to get 24-bit audio
void Signal_Buf_Process(uint8_t half);
void DMA1_Channel1_IRQHandler();
void DMA1_Channel4_IRQHandler();

// adc_buf[] half and full Interupt Flag
volatile bool signal_buf_half_ready;
volatile bool signal_buf_full_ready;

// Constants
#define USE_INMP441 1u // 1 if INMP441, 0 if MAX4466 is used.
#define SIGNAL_BUF_LEN (FFT_SIZE*2) // Double buffer for adc_buf[]
#define DMA_BUF_LEN (SIGNAL_BUF_LEN*2)
#define ADC_OFFSET 2048u // The baseline offset value for MAX4466
volatile uint16_t dma_buf[DMA_BUF_LEN]; // (*2 Length when using INMP441 since it send 24-bit data
                                                         //  so need to do two 16-bit reads on SPI2)
extern volatile uint16_t signal_buf[SIGNAL_BUF_LEN]; 


#endif // LED_DRIVER_H