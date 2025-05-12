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
