#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include "arm_math.h" // CMSIS-DSP main include
#include "arm_const_structs.h" // Contains FFT configs (critical!)

// Function Prototypes
void FFT_Init(void);
void FFT_Process(const volatile uint16_t *adc_buf, uint8_t INMP441);
q15_t Normalize_FFT_Value(q15_t val, q15_t peak_val);

// Constants
#define FFT_SIZE    512  // Choose power-of-2: 64, 128, 256, etc.
#define LOG2_FFT_SIZE 9  // log2(128) = 7


// Fixed-point input buffer (interleaved real/imaginary)
extern q15_t fft_input[2 * FFT_SIZE];

// Output magnitude buffer
extern q15_t fft_output[FFT_SIZE];

// Max magnitude for current frequency bins (for auto-scaling)
q15_t max_mag;

#endif
