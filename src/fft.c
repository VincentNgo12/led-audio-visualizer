#include "fft.h"
#include "audio_input.h"

q15_t fft_input[2 * FFT_SIZE]; // Format: [real0, imag0, real1, imag1, ...]
q15_t fft_output[FFT_SIZE];
q15_t max_mag = 0;

// CMSIS FFT config 
static const arm_cfft_instance_q15 *fft_config;

void FFT_Init(void) {
    fft_config = &arm_cfft_sR_q15_len512;
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


    // Find max magnitude (for auto-scaling)
    fft_output[0] = 0; // Remove DC component
    for (int i = 1; i < FFT_SIZE; i++) {
        if (fft_output[i] > max_mag)
            max_mag = fft_output[i];
    }
}


// Function to normalize each fft_output[] value relative to peak value
q15_t Normalize_FFT_Value(q15_t val, q15_t peak_val) {
    if (peak_val == 0) return 0; // Avoid division by 0
    int32_t scaled = ((int32_t)val << 15) / peak_val; // Normalize to Q15
    return (q15_t)__SSAT(scaled, 16); // Clamp value to 16 bit (Signed Saturation)
}

