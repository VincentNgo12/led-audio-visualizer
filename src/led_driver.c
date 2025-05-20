#include "led_driver.h"
#include "audio_input.h"
#include "stm32f1xx.h"
#include "stdint.h"
#include "fft.h"


uint8_t led_colors[NUM_LEDS][3]; // RGB for each LED
uint16_t pwm_buf[PWM_BITS]; // TIM3->CCR1 values to control PWM

volatile bool pwm_ready = false; // DMA1 Channel 3 Interupt flag

/*======================================================
  Code to initialize LED (Setup PWM with TIM3 and DMA)
======================================================*/
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
    TIM3->ARR = TIM3_PERIOD;   // Auto-reload value (ARR = 90 - 1)
    TIM3->CCR1 = 50;                   // Initial duty cycle
    TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; // PWM Mode (Output high while CNT < CCR1, low otherwise)
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;   // Enable preload
    TIM3->CCER  |= TIM_CCER_CC1E;     // Enable channel output (TIM3_CH1)
    TIM3->CR1   |= TIM_CR1_ARPE;      // Auto-reload preload enable


    /*===================================
    This section setup and configure DMA1
    =====================================*/
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN; //Enable DMA1
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;  // Disable Channel 3 before config (CCR is Channel Config Register)
    DMA1_Channel3->CPAR = (uint32_t)&TIM3->CCR1; //CPAR holds address of TIM3_CCR1 (PWD Duty Cycle)
    DMA1_Channel3->CMAR = (uint32_t)pwm_buf; // DMA1 Channel 3 points to pwd_buf[] array
    DMA1_Channel3->CNDTR = PWM_BITS; //Configure number of data element to transfer for channel (length of pwm_buf)



    DMA1_Channel3->CCR = DMA_CCR_DIR       //Direction memory to Peripheral
                        |DMA_CCR_MINC      //Memory Increment
                        |DMA_CCR_CIRC      //Circlar Mode
                        |DMA_CCR_PSIZE_0   //Peripheral data size = 16 bit
                        |DMA_CCR_MSIZE_0;  //Memory data size = 16 bit

    TIM3->DIER |= TIM_DIER_UDE; // Enable DMA for TIM3 update events (Update PWD Duty via DMA when CNT reaches ARR)
    DMA1_Channel3->CCR |= DMA_CCR_TCIE;  // Enable Transfer Complete Interrupt

    DMA1_Channel3->CCR |= DMA_CCR_EN; //Start DMA1 Channel 3
    TIM3->CR1 |= TIM_CR1_CEN; //Start TIM3_CH1

    // Enable interrupt in NVIC
    DMA1->IFCR |= DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3; //Clear DMA flag before enable IRQ
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}


/*====================================
    DMA1 Channel 3 Interupt Handler
  ====================================*/
void DMA1_Channel3_IRQHandler()
{
    if (DMA1->ISR & DMA_ISR_TCIF3) {
        DMA1->IFCR |= DMA_IFCR_CTCIF3;  // Clear interrupt flag

        pwm_ready = true; //Signal main loop (PWM Data transfer is completed)
    }
}


/*====================================
    Code to control the LED Strip
====================================*/
int magnitude_to_brightness(float magnitude) {
    float scaled = logf(magnitude + 1.0f);  // avoid log(0)
    scaled *= scale_factor;                 // tune this manually
    return clamp((int)scaled, 0, LED_MAX_BRIGHTNESS);
}


//Update led_colors given volume
void Update_Led_Colors(void) {
    // Calculate the brightness of each LED bar with associated frequency bins
    for (int bar = 0; bar < NUM_BARS; bar++) {
        float magnitude = 0.0f;
        int start_bin = 1 + bar * BINS_PER_BAR;
        int end_bin = start_bin + BINS_PER_BAR;

        for (int i = start_bin; i < end_bin; i++) {
            magnitude += fft_output[i]; // Total magnitude of current frequnency bins
        }

        magnitude /= bins_per_bar;  // average magnitude
        float brightness = magnitude_to_brightness(magnitude);

        // Update led_colors[]
        int led_idx_start = bar * LEDS_PER_BAR
        int led_idx_end = led_idx_start + LEDS_PER_BAR
        for (int i = led_idx_start; i < led_idx_end; i++) {
            led_colors[i][0] = 0x00; // Green
            led_colors[i][1] = brightness; // Red
            led_colors[i][2] = 0x00; // Blue
        }
    }
}



//Encode led_colors data into pwm_buf array (Which will be used to drive TIM3 PWM)
void Encode_Led_Data() {
    int bit_idx = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        for (int color = 0; color < 3; color++) {
            uint8_t byte = led_colors[i][color]; //Get a color byte from one color channel
            for (int bit = 7; bit >= 0; bit--) {
                //Encode each bits data into pwd_buf
                if (byte & (1 << bit)) {
                    pwm_buf[bit_idx++] = WS_HIGH;
                } else {
                    pwm_buf[bit_idx++] = WS_LOW;
                }
            }
        }
    }

    // Append reset pulse (PWM flat LOW for > 50us)
    for (int i = 0; i < RESET_SLOTS; i++) {
        pwm_buf[bit_idx++] = 0;
    }
}