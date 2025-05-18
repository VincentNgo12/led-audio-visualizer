#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Function prototypes
void LED_Init(void);
void Update_Led_Colors(void); //Update led_colors given volume
void Encode_Led_Data();

// DMA1 Channel 3 Interupt flag
volatile bool pwm_ready;

/*================================================
                LED Strip Info
================================================*/
#define NUM_LEDS 30 //Number of LEDs on strip
extern uint8_t led_colors[NUM_LEDS][3]; //RGB values for each LED


// Constants
#define LED_PWM_FREQ_HZ     800000u // 800 kHz (1.25 microsec for each bits, ARR = 89, PSC = 0, 72MHz system clock)
#define LED_PWM_RESOLUTION  90u // ARR value (90 steps)

// PWM Configuration Macros
#define TIM3_PRESCALER      0u  // No prescaling
#define TIM3_PERIOD         (LED_PWM_RESOLUTION - 1)  // ARR = 89

// PWM Controls 
#define WS_HIGH  65  // Logic 1 = ~0.8us high
#define WS_LOW   26  // Logic 0 = ~0.4us high
#define BITS_PER_LED 24
#define LED_BITS   (NUM_LEDS * BITS_PER_LED) //Total number of data bits for the LED strip
// Reset Pulse Implementation (PWM low for > 50us)
#define RESET_SLOTS 64 //(Roughly 80 us)
#define PWM_BITS (LED_BITS + RESET_SLOTS) //Full size of pwm_buf[] array

uint16_t pwm_buf[PWM_BITS]; // TIM3->CCR1 values to control PWM

#endif // LED_DRIVER_H