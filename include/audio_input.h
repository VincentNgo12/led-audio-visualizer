#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include <stdint.h>

// Function prototypes
void ADC1_Init();
uint16_t ADC1_Read();
void DMA1_Channel1_IRQHandler();
void Update_Led_Colors(uint16_t volume); //

// Constants
#define ADC_BUF_LEN 10
#define ADC_OFFSET 2047 //The baseline offset value for MAX4466
extern volatile uint16_t adc_buf[ADC_BUF_LEN];



#endif // LED_DRIVER_H