# LED Strip Audio Visualizer 🎵✨

A real-time audio visualizer using an STM32F103 microcontroller and addressable RGB LED strip. This project captures audio signals, performs FFT computations, and displays vibrant, dynamic light patterns that react to music and sound!

![Visualizer Demo](images/demo.gif) 
![Hardware Setup](images/hardware.jpg)

---

## 🚀 Features

- **Real-Time Audio Processing:** Captures audio via analog microphone (MAX4466/MAX9814).
- **FFT Spectrum Analysis:** Uses CMSIS DSP library for fast Fourier transform to analyze audio frequencies.
- **Dynamic LED Effects:** Maps frequency bands to colorful LED bars with smooth animations and peak-hold effects.
- **DMA & Interrupt Driven:** Efficient data acquisition and LED updates using DMA and hardware interrupts.
- **Customizable:** Easily adjust number of LEDs, bars, and color schemes.

---

## 🛠️ Technologies Used

- **Microcontroller:** STM32F103 (Blue Pill)
- **Programming Language:** C
- **Audio Input:**  
  - Analog: MAX4466 / MAX9814 microphone modules  
- **LED Strip:** WS2812B (NeoPixel) addressable RGB LEDs
- **Signal Processing:** CMSIS DSP library for FFT
- **Peripherals:**  
  - ADC with DMA (for analog audio)  
  - Timer-driven PWM for LED control
- **Development Environment:** Bare-metal development with CMSIS library (custom linker script, Makefile, and startup code)

---

## 📷 Gallery

| Visualizer in Action | Hardware Close-up |
|----------------------|------------------|
| ![GIF](images/action.gif) | ![Photo](images/board.jpg) |


---

## 📦 Folder Structure

```
src/
  ├── main.c           # Main application loop
  ├── audio_input.c    # Audio capture (ADC/SPI/DMA)
  ├── fft.c            # FFT processing
  ├── led_driver.c     # LED strip control
  └── ...
```

---

## 📝 How It Works

1. **Audio Capture:**  
   The microcontroller samples audio from a microphone using ADC (analog).
2. **FFT Analysis:**  
   The sampled audio is processed with a fast Fourier transform to extract frequency information.
3. **LED Mapping:**  
   Frequency bands are mapped to LED bars, with color and brightness reflecting the audio spectrum.
4. **Real-Time Display:**  
   LEDs are updated in real-time using DMA-driven PWM for smooth, flicker-free visuals.

---

## 💡 Inspiration

Inspired by music visualizers and DIY electronics, this project demonstrates real-time embedded signal processing and creative LED effects.

---

## 🙌 Acknowledgements

- [ARM CMSIS DSP Library](https://arm-software.github.io/CMSIS_5/DSP/html/index.html)
- [WS2812B LED Strip](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/cd00171190.pdf)

---

> **Made with ❤️ using STM32 and C**
