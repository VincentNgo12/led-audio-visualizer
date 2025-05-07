### Day 1
-Setup the code template using my repository
-Brainstorm which power supply will be used.

### Day 2
-Configure the system clock in system_stm32f1xx.c, switch to use HSE + PLL at 72MHZ but ran into problems.
-THe CLock frquency is incorrect.
-Had to use OpenOCD debugger to check the SystemCoreClock variable and it was still at 8MHZ.
-Turns out I didn't invoke the SystemInit() function in my startup.c at all.

### Day 3
-Did quite some work and learning today.
-Refactor Code Structure, include docs and include folders
-Setup PWD on TIM3 Channel 1 (PA6) 
-Setup and Configured DMA1 Channel 3 to Transmit PWD duty cycle data to Peripheral (TIM3)


### Day 4
-Setup ADC1_IN0 (PA0) to convert audio input from MAX4466
-Setup DMA1 Channel 1 to trasnfer ADC1 conversion to adc_buf
-Used ADC1 conversion as TIM3_CCR1 PWM Cycle
-Add debug rules to Makefile
-Find out the LED lights up after ADC1_Init() even ADC1 value is 0 (will work on this bug tomorrow)