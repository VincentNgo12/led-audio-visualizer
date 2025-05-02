#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

// Function prototypes
void LED_Init(void);

// Constants
#define LED_PWM_FREQ_HZ     720000u  // 720 kHz (for ARR = 100, PSC = 0, 72MHz clock)
#define LED_PWM_RESOLUTION  100u     // ARR value (100 steps)

// PWM Configuration Macros
#define TIM3_PRESCALER      0u  // No prescaling
#define TIM3_PERIOD         (LED_PWM_RESOLUTION - 1)  // ARR = 99


#endif // LED_DRIVER_H