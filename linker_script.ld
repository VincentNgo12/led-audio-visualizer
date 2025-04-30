/* Entry point of the program */
ENTRY(reset_handler)

/* Define memory regions based on STM32F103 specs */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K   /* 64KB Flash memory */
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 20K   /* 20KB SRAM */
}

/* Define sections and their placement in memory */
SECTIONS
{
  /* Interrupt Vector Table */
  .isr_vector :
  {
    KEEP(*(.isr_vector)) /* Keep the interrupt vector table even if unused (compilers might optimize)*/
  } > FLASH

  /* Program code and read-only data */
  .text :
  {
    . = ALIGN(4);       /* Ensure 4-byte alignment */
    
    *(.text)            /* All .text sections (code) */
    *(.rodata)          /* All .rodata sections (constants) */
    
    . = ALIGN(4);
    _etext = .;         /* End address of .text section */
  } > FLASH

  /* Initialized data */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;         /* Start address of .data section */
    
    *(.data)            /* All .data sections (initialized globals/static vars) */

    . = ALIGN(4);
    _edata = .;         /* End address of .data section */
  } > SRAM AT> FLASH    /* Load address is FLASH, execution address is SRAM */

  /* Uninitialized data (zero-initialized at runtime) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;          /* Start address of .bss section */
    
    *(.bss)             /* All .bss sections (uninitialized globals/static vars) */
    
    . = ALIGN(4);
    _ebss = .;          /* End address of .bss section */
  } > SRAM
}
