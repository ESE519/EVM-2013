/* Startup file to be used with GCC / ARM Cortex-M
 *
 * Copyright 2013 The MathWorks, Inc.
 */

#define WEAK __attribute__ ((weak))

/*----------Stack Configuration-----------------------------------------------*/
#define __STACK_SIZE       0x200      /*!< Stack size (in Words)           */
__attribute__ ((section(".co_stack")))
unsigned long pulStack[__STACK_SIZE + (((STACK_SIZE>>2) + 7) & ~7)];

void WEAK ResetISR(void);
static void NmiSR(void);
static void FaultISR(void);
static void IntDefaultHandler(void);
extern void WEAK SysTick_Handler(void);
extern int main(void);


/*----------Symbols defined in linker script----------------------------------*/
extern unsigned long _sidata;    /*!< Start address for the initialization
 * values of the .data section.            */
extern unsigned long _sdata;     /*!< Start address for the .data section     */
extern unsigned long _edata;     /*!< End address for the .data section       */
extern unsigned long _sbss;      /*!< Start address for the .bss section      */
extern unsigned long _ebss;      /*!< End address for the .bss section        */
extern void _eram;               /*!< End address for ram                     */

/* Interrupt vector table */
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((unsigned long)pulStack + sizeof(pulStack)),
    // The initial stack pointer
    ResetISR,                               // The reset handler
    NmiSR,                                  // The NMI handler
    FaultISR,                               // The hard fault handler
    IntDefaultHandler,                      // The MPU fault handler
    IntDefaultHandler,                      // The bus fault handler
    IntDefaultHandler,                      // The usage fault handler
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // SVCall handler
    IntDefaultHandler,                      // Debug monitor handler
    0,                                      // Reserved
    IntDefaultHandler,                      // The PendSV handler
    SysTick_Handler,                        // The SysTick handler
    IntDefaultHandler,                      // GPIO Port A
    IntDefaultHandler,                      // GPIO Port B
    IntDefaultHandler,                      // GPIO Port C
    IntDefaultHandler,                      // GPIO Port D
    IntDefaultHandler,                      // GPIO Port E
    IntDefaultHandler,                      // UART0 Rx and Tx
    IntDefaultHandler,                      // UART1 Rx and Tx
    IntDefaultHandler,                      // SSI0 Rx and Tx
    IntDefaultHandler,                      // I2C0 Master and Slave
    IntDefaultHandler,                      // PWM Fault
    IntDefaultHandler,                      // PWM Generator 0
    IntDefaultHandler,                      // PWM Generator 1
    IntDefaultHandler,                      // PWM Generator 2
    IntDefaultHandler,                      // Quadrature Encoder 0
    IntDefaultHandler,                      // ADC Sequence 0
    IntDefaultHandler,                      // ADC Sequence 1
    IntDefaultHandler,                      // ADC Sequence 2
    IntDefaultHandler,                      // ADC Sequence 3
    IntDefaultHandler,                      // Watchdog timer
    IntDefaultHandler,                      // Timer 0 subtimer A
    IntDefaultHandler,                      // Timer 0 subtimer B
    IntDefaultHandler,                      // Timer 1 subtimer A
    IntDefaultHandler,                      // Timer 1 subtimer B
    IntDefaultHandler,                      // Timer 2 subtimer A
    IntDefaultHandler,                      // Timer 2 subtimer B
    IntDefaultHandler,                      // Analog Comparator 0
    IntDefaultHandler,                      // Analog Comparator 1
    IntDefaultHandler,                      // Analog Comparator 2
    IntDefaultHandler,                      // System Control (PLL, OSC, BO)
    IntDefaultHandler                       // FLASH Control
};


//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************
void ResetISR(void)
{
    /* Initialize data and bss */
    unsigned long *pulSrc, *pulDest;
    
    /* Copy the data segment initializers from flash to SRAM */
    pulSrc = &_sidata;
    
    for(pulDest = &_sdata; pulDest < &_edata; ) {
        *(pulDest++) = *(pulSrc++);
    }
    
    /* Zero fill the bss segment.  This is done with inline assembly since this
     will clear the value of pulDest if it is not kept in a register. */
    __asm("  ldr     r0, =_sbss\n"
            "    ldr     r1, =_ebss\n"
            "    mov     r2, #0\n"
            "    .thumb_func\n"
            "zero_loop:\n"
            "        cmp     r0, r1\n"
            "        it      lt\n"
            "        strlt   r2, [r0], #4\n"
            "        blt     zero_loop");
    /* Call system clock initialization code */
#ifndef __NO_SYSTEM_INIT
    SystemInit();
#endif
    
    /* Call the application's entry point.*/
    main();
}

/* Non-maskable interrupt */
static void NmiSR(void)
{
    while(1)
    {
    }
}

/* Fault interrupt */
static void FaultISR(void)
{
    while(1)
    {
    }
}

/* Default interrupt handler */
static void IntDefaultHandler(void)
{
    while(1)
    {
    }
}
