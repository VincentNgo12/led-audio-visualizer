#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include <stdint.h>

// Function prototypes
void ADC1_Init();
uint16_t ADC1_Read();
void DMA1_Channel1_IRQHandler();

// Constants
#define ADC_BUF_LEN 1
extern volatile uint16_t adc_buf[ADC_BUF_LEN];



#endif // LED_DRIVER_H