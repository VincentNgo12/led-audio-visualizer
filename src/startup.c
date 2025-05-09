/* * startup.c * STM32F103 Startup code
 * - Sets up the interrupt vector table
 * - Initializes .data and .bss sections
 * - Calls main()
 */

 #include <stdint.h>

 /* Define memory layout (matching linker script) */
 #define SRAM_START (0x20000000U)
 #define SRAM_SIZE  (20U * 1024U)            /* 20 KB SRAM */
 #define SRAM_END   (SRAM_START + SRAM_SIZE)
 #define STACK_POINTER_INIT_ADDRESS (SRAM_END) //Stack starts at the end of SRAM
 
 /* Size of ISR vector table (in words) */
 #define ISR_VECTOR_SIZE_WORDS 114  // Adjust according to device
 
 /* Forward declaration of exception and interrupt handlers */
 void reset_handler(void);
 void default_handler(void);
 
 /* Weak aliases for interrupt handlers to default_handler unless overridden */
 /* System exception vector handler */
 void nmi_handler(void)           __attribute__((weak, alias("default_handler")));
 void hard_fault_handler(void)    __attribute__((weak, alias("default_handler")));
 void bus_fault_handler(void)     __attribute__((weak, alias("default_handler")));
 void usage_fault_handler(void)   __attribute__((weak, alias("default_handler")));
 void svcall_handler(void)        __attribute__((weak, alias("default_handler")));
 void debug_monitor_handler(void) __attribute__((weak, alias("default_handler")));
 void pendsv_handler(void)        __attribute__((weak, alias("default_handler")));
 void systick_handler(void)       __attribute__((weak, alias("default_handler")));
 
 /* Specific peripheral interrupt handlers */
 void WWDG_IRQHandler(void)       __attribute__((weak, alias("default_handler")));
 void PVD_IRQHandler(void)        __attribute__((weak, alias("default_handler")));
 void TAMPER_IRQHandler(void)     __attribute__((weak, alias("default_handler")));
 void RTC_IRQHandler(void)        __attribute__((weak, alias("default_handler")));
 void FLASH_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void RCC_IRQHandler(void)        __attribute__((weak, alias("default_handler")));
 void EXTI0_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void EXTI1_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void EXTI2_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void EXTI3_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void EXTI4_IRQHandler(void)      __attribute__((weak, alias("default_handler")));
 void DMA1_Channel1_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel2_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel3_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel4_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel5_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel6_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void DMA1_Channel7_IRQHandler(void)    __attribute__((weak, alias("default_handler")));
 void ADC1_2_IRQHandler(void)           __attribute__((weak, alias("default_handler")));
 void USB_HP_CAN1_TX_IRQHandler(void)   __attribute__((weak, alias("default_handler")));
 void USB_LP_CAN1_RX0_IRQHandler(void)  __attribute__((weak, alias("default_handler")));
 void CAN1_RX1_IRQHandler(void)         __attribute__((weak, alias("default_handler")));
 void CAN1_SCE_IRQHandler(void)         __attribute__((weak, alias("default_handler")));
 void EXTI9_5_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void TIM1_BRK_IRQHandler(void)         __attribute__((weak, alias("default_handler")));
 void TIM1_UP_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void TIM1_TRG_COM_IRQHandler(void)     __attribute__((weak, alias("default_handler")));
 void TIM1_CC_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void TIM2_IRQHandler(void)             __attribute__((weak, alias("default_handler")));
 void TIM3_IRQHandler(void)             __attribute__((weak, alias("default_handler")));
 void TIM4_IRQHandler(void)             __attribute__((weak, alias("default_handler")));
 void I2C1_EV_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void I2C1_ER_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void I2C2_EV_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void I2C2_ER_IRQHandler(void)          __attribute__((weak, alias("default_handler")));
 void SPI1_IRQHandler(void)             __attribute__((weak, alias("default_handler")));
 void SPI2_IRQHandler(void)             __attribute__((weak, alias("default_handler")));
 void USART1_IRQHandler(void)           __attribute__((weak, alias("default_handler")));
 void USART2_IRQHandler(void)           __attribute__((weak, alias("default_handler")));
 void USART3_IRQHandler(void)           __attribute__((weak, alias("default_handler")));
 void EXTI15_10_IRQHandler(void)        __attribute__((weak, alias("default_handler")));
 void RTCAlarm_IRQHandler(void)         __attribute__((weak, alias("default_handler")));
 void USBWakeUp_IRQHandler(void)        __attribute__((weak, alias("default_handler"))); 

 
 /* ISR vector table */
 uint32_t isr_vector[ISR_VECTOR_SIZE_WORDS] __attribute__((section(".isr_vector"))) = {
    STACK_POINTER_INIT_ADDRESS,     /* Initial Stack Pointer */
    (uint32_t)&reset_handler,        /* Reset Handler */
    (uint32_t)&nmi_handler,          /* NMI Handler */
    (uint32_t)&hard_fault_handler,   /* Hard Fault Handler */
    (uint32_t)&bus_fault_handler,    /* Bus Fault Handler */
    (uint32_t)&usage_fault_handler,  /* Usage Fault Handler */
    0,                               /* Reserved */
    0,                               /* Reserved */
    0,                               /* Reserved */
    0,                               /* Reserved */
    0,                               /* Reserved */
    (uint32_t)&svcall_handler,       /* SVCall Handler */
    (uint32_t)&debug_monitor_handler,/* Debug Monitor Handler */
    0,                               /* Reserved */
    (uint32_t)&pendsv_handler,       /* PendSV Handler */
    (uint32_t)&systick_handler,      /* SysTick Handler */

     /* Device-specific interrupt handlers start here*/
    (uint32_t)&WWDG_IRQHandler,         // 0
    (uint32_t)&PVD_IRQHandler,          // 1
    (uint32_t)&TAMPER_IRQHandler,       // 2
    (uint32_t)&RTC_IRQHandler,          // 3
    (uint32_t)&FLASH_IRQHandler,        // 4
    (uint32_t)&RCC_IRQHandler,          // 5
    (uint32_t)&EXTI0_IRQHandler,        // 6
    (uint32_t)&EXTI1_IRQHandler,        // 7
    (uint32_t)&EXTI2_IRQHandler,        // 8
    (uint32_t)&EXTI3_IRQHandler,        // 9
    (uint32_t)&EXTI4_IRQHandler,        // 10
    (uint32_t)&DMA1_Channel1_IRQHandler,// 11
    (uint32_t)&DMA1_Channel2_IRQHandler,// 12
    (uint32_t)&DMA1_Channel3_IRQHandler,// 13
    (uint32_t)&DMA1_Channel4_IRQHandler,// 14
    (uint32_t)&DMA1_Channel5_IRQHandler,// 15
    (uint32_t)&DMA1_Channel6_IRQHandler,// 16
    (uint32_t)&DMA1_Channel7_IRQHandler,// 17
    (uint32_t)&ADC1_2_IRQHandler,       // 18
    (uint32_t)&USB_HP_CAN1_TX_IRQHandler, // 19
    (uint32_t)&USB_LP_CAN1_RX0_IRQHandler,// 20
    (uint32_t)&CAN1_RX1_IRQHandler,     // 21
    (uint32_t)&CAN1_SCE_IRQHandler,     // 22
    (uint32_t)&EXTI9_5_IRQHandler,      // 23
    (uint32_t)&TIM1_BRK_IRQHandler,     // 24
    (uint32_t)&TIM1_UP_IRQHandler,      // 25
    (uint32_t)&TIM1_TRG_COM_IRQHandler, // 26
    (uint32_t)&TIM1_CC_IRQHandler,      // 27
    (uint32_t)&TIM2_IRQHandler,         // 28
    (uint32_t)&TIM3_IRQHandler,         // 29
    (uint32_t)&TIM4_IRQHandler,         // 30
    (uint32_t)&I2C1_EV_IRQHandler,      // 31
    (uint32_t)&I2C1_ER_IRQHandler,      // 32
    (uint32_t)&I2C2_EV_IRQHandler,      // 33
    (uint32_t)&I2C2_ER_IRQHandler,      // 34
    (uint32_t)&SPI1_IRQHandler,         // 35
    (uint32_t)&SPI2_IRQHandler,         // 36
    (uint32_t)&USART1_IRQHandler,       // 37
    (uint32_t)&USART2_IRQHandler,       // 38
    (uint32_t)&USART3_IRQHandler,       // 39
    (uint32_t)&EXTI15_10_IRQHandler,    // 40
    (uint32_t)&RTCAlarm_IRQHandler,     // 41
    (uint32_t)&USBWakeUp_IRQHandler     // 42
 };
 
 /* Default interrupt handler */
 void default_handler(void)
 {
     while (1)
     {
         /* Infinite loop to capture unexpected interrupts */
     }
 }
 
 /* Extern symbols defined in the linker script */
 extern uint32_t _etext, _sdata, _edata, _sbss, _ebss;
 
 /* Main application entry point */
 int main(void);
 extern void SystemInit(void); //SystemInit() from system_stm32f1xx.c
 
 /* Reset handler: program entry after MCU reset */ void reset_handler(void)
 {
     /* Copy .data section from Flash to SRAM */
     uint32_t data_size = (uint32_t)&_edata - (uint32_t)&_sdata;
     uint8_t *flash_data = (uint8_t *)&_etext;
     uint8_t *sram_data  = (uint8_t *)&_sdata;
     
     for (uint32_t i = 0; i < data_size; i++)
     {
         sram_data[i] = flash_data[i];
     }
 
     /* Zero initialize the .bss section in SRAM */
     uint32_t bss_size = (uint32_t)&_ebss - (uint32_t)&_sbss;
     uint8_t *bss = (uint8_t *)&_sbss;
 
     for (uint32_t i = 0; i < bss_size; i++)
     {
         bss[i] = 0;
     }
 
     /* Call the application's entry point */
     SystemInit(); //Invoke SystemInit() to use the system clock configurations in system_stm32f1xx.c
     main();
 }
 

//Prevent crashing in case main() was not defined
__attribute__((weak)) int main(void)
{
    while(1);
}