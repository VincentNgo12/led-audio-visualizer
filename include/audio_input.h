#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include "fft.h"
#include <stdint.h>
#include <stdbool.h>

// Function prototypes
void ADC1_Init();
uint16_t ADC1_Read();
void ADC_Buf_Process(uint8_t half);
void DMA1_Channel1_IRQHandler();

// adc_buf[] half and full Interupt Flag
volatile bool adc_buf_half_ready;
volatile bool adc_buf_full_ready;

// Constants
#define ADC_BUF_LEN (FFT_SIZE*2) // Double buffer for adc_buf[]
#define ADC_OFFSET 2048u // The baseline offset value for MAX4466
extern volatile uint16_t adc_buf[ADC_BUF_LEN];



#endif // LED_DRIVER_H