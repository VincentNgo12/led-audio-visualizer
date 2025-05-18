#include "fft.h"
#include "audio_input.h"

q15_t fft_input[2 * FFT_SIZE]; // Format: [real0, imag0, real1, imag1, ...]
q15_t fft_output[FFT_SIZE];

// CMSIS FFT config 
static const arm_cfft_instance_q15 *fft_config;

void FFT_Init(void) {
    fft_config = &arm_cfft_sR_q15_len128;
}

void FFT_Process(const volatile uint16_t *adc_buf) {
    // Step 1: Convert ADC data to Q15 format (centered around zero)
    for (int i = 0; i < FFT_SIZE; i++) {
        int16_t centered = ((int16_t)adc_buf[i]) - ADC_OFFSET; // 12-bit centered
        fft_input[2 * i]     = (q15_t)(centered << 3);   // Q15 scaling
        fft_input[2 * i + 1] = 0;                        // Every Imag part = 0
    }

    // Step 2: Perform in-place FFT
    arm_cfft_q15(fft_config, fft_input, 0, 1);  // forward, bit-reversal enabled

    // Step 3: Compute magnitude
    arm_cmplx_mag_q15(fft_input, fft_output, FFT_SIZE);
}
