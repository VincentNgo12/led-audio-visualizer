#include "led_driver.h"
#include "audio_input.h"
#include "stm32f1xx.h"
#include "stdint.h"
#include "fft.h"
#include "log_lut.h"


uint8_t led_colors[NUM_LEDS][3]; // RGB for each LED
uint16_t led_bar_levels_q[NUM_BARS] = {0}; // Fixed-point (Q4.4)
uint16_t pwm_buf[PWM_BITS]; // TIM3->CCR1 values to control PWM

volatile bool pwm_ready = false; // DMA1 Channel 3 Interupt flag
static uint16_t hue_offset_q = 0; // Hue offset for LED colors (Q4.4)
#define FRACTION_SHIFT 4 // (To implement Q4.4)

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
//Update led_colors given volume
void Update_Led_Colors(void) {
    uint16_t led_idx = 0; // Number of LEDs updated
    // Calculate the brightness of each LED bar with associated frequency bins
    for (uint8_t bar = 0; bar < NUM_BARS; bar++) {
        int32_t magnitude = 0; // Uses int32_t to avoid overflow during q15 calculations
        int start_bin = 1 + bar * BINS_PER_BAR;
        int end_bin = start_bin + BINS_PER_BAR;

        for (int i = start_bin; i < end_bin; i++) {
            q15_t norm_val = Normalize_FFT_Value(fft_output[i], max_mag); // Get normalized magnitude
            magnitude += (norm_val < 0) ? -norm_val : norm_val; // Total normalized magnitude of current frequnency bins (Abs value)
        }

        q15_t avg_magnitude = (q15_t)(magnitude / BINS_PER_BAR);  // Average magnitude (back to Q15)
        uint8_t brightness = Magnitude_To_Brightness_q15(avg_magnitude, max_mag); // Brightness based on magnitude
        brightness = Set_Bar_Levels(brightness, bar); // Brightness after updated LED bar levels
        uint16_t bar_height = Get_Bar_Height(brightness); // Bar height based on brightness

        // Update led_colors[] for current bar
        uint16_t led_idx_start = bar * LEDS_PER_BAR;
        for (uint16_t i = 0; i < LEDS_PER_BAR; i++) {
            uint16_t index = led_idx_start + i;
            uint8_t hue_offset = (uint8_t)(hue_offset_q >> FRACTION_SHIFT);
            uint8_t hue = ((i * 150) / LEDS_PER_BAR + hue_offset) & 0xFF; // Maps to hue range 0–255 
            uint8_t r, g, b;
            HSV_To_RGB(hue, 255, brightness, &r, &g, &b);            

            if (i < bar_height) {
                led_colors[index][0] = g; // Green
                led_colors[index][1] = r;       // Red
                led_colors[index][2] = b;       // Blue
            } else {
                // Turn off unused LEDs in the bar
                led_colors[index][0] = 0x00;
                led_colors[index][1] = 0x00;
                led_colors[index][2] = 0x00;
            }
            led_idx++;
        } 
    }

    if (led_idx == NUM_LEDS) return; // Return if all LEDs are updated

    // Turn off unused LEDs
    for (int i = led_idx; i < NUM_LEDS; i++) {
        led_colors[i][0] = 0x00; // Green
        led_colors[i][1] = 0x00; // Red
        led_colors[i][2] = 0x00; // Blue
    }    

    hue_offset_q += HUE_OFFSET_RATE; // Change hue over time
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


// Get brightness based on abverage signal magnitude
uint8_t Magnitude_To_Brightness_q15(q15_t mag_q15, q15_t max_mag) {
    if (max_mag == 0) return 0; // avoid divide by 0
    uint16_t abs_mag = (mag_q15 < 0) ? -mag_q15 : mag_q15; // abs_mag: 0.0 to 1.0
    uint32_t scaled = ((uint32_t)abs_mag << 15) / max_mag;  // Now scaled from 0 to 32767
    uint8_t index = (scaled * (LUT_SIZE - 1)) >> 15;  // scale 0 to LUT_SIZE
    return log_lut[index]; // Return the associated brightness (from Look-up table) }
}

// Set bar levels based on new brightness and return updated brightness
uint8_t Set_Bar_Levels(uint8_t new_brightness, uint8_t bar_idx) {
    uint16_t new_brightness_q = ((uint16_t)new_brightness) << DECAY_SHIFT; // Convert to fixed-point
    uint16_t current_level_q = led_bar_levels_q[bar_idx];
    // Apply a "peak hold + decay" behavior
    if (new_brightness_q > current_level_q) {
        current_level_q = new_brightness_q; // jump up quickly
    } else if (current_level_q >= BRIGHTNESS_DECAY_Q){
        current_level_q -= BRIGHTNESS_DECAY_Q; // Slow decay
    } else {
        current_level_q = 0;
    }    

    led_bar_levels_q[bar_idx] = current_level_q; // Update led_bar_levels[]
    new_brightness = (uint8_t)(current_level_q >> DECAY_SHIFT); // Convert back to normal 8-bit brightness

    return new_brightness;
}

// Get Bar height based on brightness
uint16_t Get_Bar_Height(uint8_t brightness) {
    if (brightness == 0) return 0;

    uint16_t height = (brightness * LEDS_PER_BAR) / (LED_MAX_BRIGHTNESS);

    return height;
}


// HSV to RGB converter
void HSV_To_RGB(uint8_t h, uint8_t s, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b){
    uint8_t region = h / 43; // 256 / 6 ≈ 43 (Divide into 6 regions of Hue colors)
    uint8_t remainder = (h - (region * 43)) * 6; // How far between two colors

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0: *r = v; *g = t; *b = p; break; // Red → Yellow
        case 1: *r = q; *g = v; *b = p; break; // Yellow → Green
        case 2: *r = p; *g = v; *b = t; break; // Green → Cyan
        case 3: *r = p; *g = q; *b = v; break; // Cyan → Blue
        case 4: *r = t; *g = p; *b = v; break; // Blue → Magenta
        default:*r = v; *g = p; *b = q; break; // Magenta → Red 
    }    
}