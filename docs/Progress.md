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


### Day 9
-Start debugging.
-PA13 didn't even light on so the program didn't even reach main()
-Found out with GDB that the program crashed at reset_handler(), dunno why
-arm-none-eabi-size shows that code size is not the problem, no overflow but reset_handler() stall at push {r3, lr}
-Maybe it has to do with the optimization flags of the compiler (-flto)
-Adding __attribute__((naked, used)) to reset_handler() fixed the issue, but the program still don't reach main().
-Now the program stall at SystemInit() inside reset_handler()
-removed -flto from startup.o and system_stm32f1xx.o
-That -flto flag is definitely the culprit, even affecting main.c 
-That -flto is causing so much trouble, affecting so much files. I need to come to a solution tomorrow.
-At least I know exactly what the problem is now.


### Day 10
-Decided to change Makefile to only apply -flto -Os to those CMSIS-DSP files.
-Seems to be working now, should have come up with this earlier
-Current ADC1 sample rate is ~142,857 samples/sec
-Switch to using TIM2 to trigger ADC1 capture at 43.47 kHz sample rate
-Start setting up header constant to divide LED strip into bars to visualize
-Setup general Update_Led_Colors() but need more work.
***TODO: +Map frequency bins magnitude into LED bars brightness
         +Use q15_t to do numerical calculations
         +Assign unique colors to each LED bars based on frequency bins magnitude

    
### Day 11
-Scaled frequencies magnitude to appropriate LED brightness.
-Assign frequnecy bins to visual bars.
-Seems to work very well, however unused LEDs are always full white which will draw lots of current. Need to fix that.
-Tested with a DC powersupply, oscilloscope, and a function generator.
-The FFT is working amazingly, its a charm.
-The LED bar are divided perfectly for each frequency bins too!
-However, the oscilloscope is telling that the MAX4466 is very insensitive, it's having a really hard time picking up sounds.
*** TODO: Come up with a way for a more sensitive microphone.


### Day 12
-Tried the Big Sound sensor with the oscilloscope, no luck.
-Can't stick with MAX4466 either, too insenstive.
-Code to turn off unsued LEDs
-Added peak hold and decay behaviour for each LEDs bars
-Dynamic LED bar height
-Didn't work beautiful quite yet, bar height are jumping crazy
-The decay rate of 1 is too fast for 72MHz.
-Obviously can't use float for decay rate, need to come up with something.
-Something has gone crazy ADC is not at constant 0 when it should be.
-Turns out I was using unsigned int for brightness so when it decay past zero, the value overflowed
-Found out the Log_LUT table doesn't even have the maximum value of 255.


### Day 13
-Fixed the underflow problem
-Having a real bad day, realized that the function generator is producing messy signal even the oscilloscope can't read the frequnecy.
-Spend too much time on this issue, but if it's not fixed I can't move on with the project testing.
-I'm sure the damn log() lookup table are incorrect too! (it was pretty correct actually)
-Added 4x gain to the FFT magnitude, I think that's the problem.
-Added min bar height
-Something is very wrong with the loop to set bar height
-Fixed so that LEDs over bar_height will be turned off, didn't realize this was quite a big bug
-Really bad day today, spend so much time and nothing done
*** TODO: +Smoother Decay Rate
          +Get better microphone
          +

### Day 14
-Will work on the color gradient today and maybe the smoother decay rate.
-Its working, but since the hue_offset is int, its incrementing too fast so the colors are changing too rapidly.
-I'm going to fix this along with the Decay Rate
-Used Q4.4 fixed point to have a more gradual decay for the brightness levels,
-The Hue decay rate is now good too, I'm very pleased with the color gradient results (It's pretty)
*** TODO: + Fix Get_Bar_Height() to assign heights more cleanly
          + Figure out the expected ADC signal range to process more cleanly. 


### Day 15
-Tried to address the FFT magnitude problem to determine the bar height more accurately.
-ADC signal range is expected to be between 0 and 4095 max.
-Implemented auto-scaling for FFT magnitudes but doesn't seems to help much. The bar level is still 1 at most.
-I believe its the log_lut[] fault again
-Updated log_lut[] and now there is an underflow problem and I dont know where it is
-log_lut[] approach is definitely not reliable, gotta address that tomorrow and potentially replace the method.


### Day 16
-Tried to normalize with max average magnitude of each bins.
-Its not working
-Fixed the underflow problem, Set_Bar_Levels() took an uint8_t but interpreted it as int8_t, causing negative values
-Implement Normalize_FFT_Value() to normalize each magnitude relative to the max magnitude
-Changed brightness decay to 40 (x10 times) much better this way.
*** TODO: Address the log_lut[]


### Day 17
-Used online C compiler to generate accurate log_lut[], but its too bright now.
-Implemted alternating vertial bar's bottom to solder strip onto board.
-Switching to INMP441 is going to be quite lots of work.
-Tomorrow I will start soldering the led strip onto the board.
*** TODO: 1. Setup SPI2 (receive-only master mode)
          2. Set up DMA for SPI2 RX
          3. Configure it to capture audio from INMP441 at 44.1 kHz (approx.)
          4. Configure Clock for SPI2 to Generate BCLK
          5. Update FFT Input


### Day 18
-Started building the led visualizer board.
-Found a very perfect 27x15.9 cm wooden board fit perfectly for my 5m IP30 led strip.
-Soldering part was painful, messed up one strip but learned experience for the other strips instead.
-That broken strip will be replaced later on.



### Day 19
-Start switching to the INMP441 digital microphone.
-renamed all ADC and signal components.
-Copied code for SPI2 and DMA1 Channel 4 Initialization.
-However, still need to address the SPI2 baud rate and reading INMP441's 24-bit data with SPI2's 16-bit.
-Also find out if we need to control the WS pin.
-Need to create dma_buf[] and DMA_Buf_Process() to process dma reads into 24-bit audio data in signal_buf[]
-Need to also reimplement the double buffer thing.
-Need to check fft.c again to ensure compatibality with new audio data
-First try seems to work but the leds light up all over the place randomly. Need to start debugging to see if INMP441 is working correctly.
-Yeah, I forget to use SPI2 amd DMA1 channel 4 instead of ADC1...
-main.c also freezes somewhere...
-It freezes as soon as SPI2 is enabled
-Removing the line SPI2->CR1 |= SPI_CR1_RXONLY; fixes the issue.
-Now it freezing becasue the signal_buf[] is not raising any flags.
-I can fix this but its late now so tomorrow...
*** TODO: check signal_buf[] flag


### Day 20
-Debug signal_buf[] flag again
-Turns out DMA1_Channel4_IRQHandler() was checking DMA1 Channel 1 flag the whole time.
-Didn't fix it actually, the programm never reach DMA1_Channel4_IRQHandler() at all.
-I forget to set the DMA1 Channel 4 memory address from signal_buf[] to dma_buf[]
-Still no data is being trasnfered into dma_buf[]
-THe line  // SPI2->CR1 |= SPI_CR1_RXONLY;   // Receive only mode is the root of the problem. Removing it save the program from freezing but now there is no DMA activity.
-Will address this tomorrow, I gotta solder the LED board today.
*** TODO: Address the SPI2 receive only mode


### Day 21
-Continue to debug INMP441 and SPI2.
-Appearently I need to write dummy data into SPI2->DR everytime I receive data from INMP441 in order to drive the SPI2 SCK clock. This way, the DMA1 Channel 4 would start transfering data.
-DMA1 Channel 4 is working fine now but INMP441 only outpts 0 constantly.
-signal_buf[] and fft_output[] have mostly zero but also some random value here and there.
-Something is writting grabage values to signal_buf[], Process_DMA_Buf() is working correctly...
-The garbage values came from this line in fft.c: arm_cfft_q15(fft_config, fft_input, 0, 1);  // forward, bit-reversal enabled
-******OMG, I never called FFT_Init() inside INMP441_Init(), this bug is a big pain in the bump! SOOO hard to catch!
-Now I have to addrress why INMP441 always output 0 value.
*** TODO: +Confirm SCK is 2.25 Mhz
          +Toggle WS before reading