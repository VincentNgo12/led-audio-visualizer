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
 void nmi_handler(void)           __attribute__((weak, alias("default_handler")));
 void hard_fault_handler(void)    __attribute__((weak, alias("default_handler")));
 void bus_fault_handler(void)     __attribute__((weak, alias("default_handler")));
 void usage_fault_handler(void)   __attribute__((weak, alias("default_handler")));
 void svcall_handler(void)        __attribute__((weak, alias("default_handler")));
 void debug_monitor_handler(void) __attribute__((weak, alias("default_handler")));
 void pendsv_handler(void)        __attribute__((weak, alias("default_handler")));
 void systick_handler(void)       __attribute__((weak, alias("default_handler")));
 
 /* TODO: Add specific peripheral interrupt handlers if needed (e.g., USART1_IRQHandler) */
 
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
     /* Device-specific interrupt handlers can be added here */
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