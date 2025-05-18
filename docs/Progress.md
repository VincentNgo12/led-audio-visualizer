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


### Day 5
-Fixed Makefile for debug mode
-Spent a lot of time with GDB debbuger to fix the bug today.
-Spent hours to debug the DMA1 Channel 1 bug. Turns out it was the ADC1 after. It's the damn trick to turn the ADC1 on twice after configuration.
-Today has nothing done except debugging.
-Got the ADC and DMA to work but find out that DMA interupt is not working.
-Spent more time debugging.
-Propbably because the isr_vector defined in startup.c is insufficient
-Defined a whole bunch more interupt handlers in startup.c
-It's working now, turns out the DMA1 interupt handler was also part of the big bug today.
-Proud of myself.


### Day 6
-Did nothing today really.
-Finished setting up the LED strip interface with PWM but doubted its going to work.
-It didn't work, don't even know where to start debugging.
-After playing around with the electronic testing devices, I found out that my PMW signal is working just fine.
-The power source seems to deliver 5.1V too.
-But the LED strip is still off.
-Later I found out that it was because I didn't implement the reset pulse, that's why the led strip didn't latch on to new data.
-I was so happy that it worked.
-TODO: Implement reset pulse


### Day 7
-Implemented the Reset Pulse for the LED Strip
-Should be transfering LED data okay now.
-The response suffer massive delay and lags, unpredictable really, gotta fix that.
-The color data is ordered as GRB...
-I forgot to check if the DMA trasnfer from pwm_buf to TIM3->CCR1 is completed before trasnfer LED data.
-FIxed that with DMA1 Channel 3 interupt flag
-Turns out the reason the LED was unresponsive and lagging because I have a massive 1 second delay in the main loop
***TODO: Implement Double ADC Buffer
-Implemented Double ADC Buffer with DMA1 half and full interupt
-I now have a very responsive LED strip
-Tried to install CMSIS-DSP library, have problems with Makefile build rules again.
-Will try to fix the Makefile to include and compile CMSIS-DSP library tomorrow.
***TODO: Fix Makefile to install CMSIS-DSP
***TODO: Implement FFT with CMSIS-DSP


### Day 8
-Setup FFT and tried to test it on the LED strip
-Ran into a bunch of compilation and include errors
-Switch from arm_cfft_radix4_q15 to arm_cfft_q15
-Compliation errors took a bunch of time to fix.
-Ran into a massive bug, TIM3 PWM is not responding at all. DOn't even know where to start debugging.
-Will try tomorrow