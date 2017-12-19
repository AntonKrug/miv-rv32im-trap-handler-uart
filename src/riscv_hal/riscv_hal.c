/*******************************************************************************
 * (c) Copyright 2016-2017 Microsemi SoC Products Group. All rights reserved.
 *
 * @file riscv_hal.c
 * @author Microsemi SoC Products Group
 * @brief Implementation of Hardware Abstraction Layer for RISC-V soft processor
 *        CoreRISCV_AXI4.
 *        This is mainly targeted at RV32IM but should be usable with other
 *        variants.
 *
 * SVN $Revision: 9187 $
 * SVN $Date: 2017-05-13 13:31:28 +0530 (Sat, 13 May 2017) $
 */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#if defined(DETAIL_TRAP_MESSAGES) && defined(MSCC_STDIO_THRU_CORE_UART_APB)
#include <string.h>
#endif


#include "riscv_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_PRESCALER 100

#define SUCCESS 0
#define ERROR   1

/*------------------------------------------------------------------------------
 * 
 */
uint8_t Invalid_IRQHandler(void);
uint8_t External_1_IRQHandler(void);
uint8_t External_2_IRQHandler(void);
uint8_t External_3_IRQHandler(void);
uint8_t External_4_IRQHandler(void);
uint8_t External_5_IRQHandler(void);
uint8_t External_6_IRQHandler(void);
uint8_t External_7_IRQHandler(void);
uint8_t External_8_IRQHandler(void);
uint8_t External_9_IRQHandler(void);
uint8_t External_10_IRQHandler(void);
uint8_t External_11_IRQHandler(void);
uint8_t External_12_IRQHandler(void);
uint8_t External_13_IRQHandler(void);
uint8_t External_14_IRQHandler(void);
uint8_t External_15_IRQHandler(void);
uint8_t External_16_IRQHandler(void);
uint8_t External_17_IRQHandler(void);
uint8_t External_18_IRQHandler(void);
uint8_t External_19_IRQHandler(void);
uint8_t External_20_IRQHandler(void);
uint8_t External_21_IRQHandler(void);
uint8_t External_22_IRQHandler(void);
uint8_t External_23_IRQHandler(void);
uint8_t External_24_IRQHandler(void);
uint8_t External_25_IRQHandler(void);
uint8_t External_26_IRQHandler(void);
uint8_t External_27_IRQHandler(void);
uint8_t External_28_IRQHandler(void);
uint8_t External_29_IRQHandler(void);
uint8_t External_30_IRQHandler(void);
uint8_t External_31_IRQHandler(void);

/*------------------------------------------------------------------------------
 * 
 */
extern void handle_m_ext_interrupt();
extern void handle_m_timer_interrupt();
extern void Software_IRQHandler();

/*------------------------------------------------------------------------------
 * Increment value for the mtimecmp register in order to achieve a system tick
 * interrupt as specified through the SysTick_Config() function.
 */
static uint64_t g_systick_increment = 0;

/*------------------------------------------------------------------------------
 * Disable all interrupts.
 */
void __disable_irq(void)
{
    clear_csr(mstatus, MSTATUS_MPIE);
    clear_csr(mstatus, MSTATUS_MIE);
}

/*------------------------------------------------------------------------------
 * Enabler all interrupts.
 */
void __enable_irq(void)
{
    set_csr(mstatus, MSTATUS_MIE);
}

/*------------------------------------------------------------------------------
 * Configure the machine timer to generate an interrupt.
 */
uint32_t SysTick_Config(uint32_t ticks)
{
    uint32_t ret_val = ERROR;
    
    g_systick_increment = ticks / RTC_PRESCALER;

    if (g_systick_increment > 0)
    {
        uint32_t mhart_id = read_csr(mhartid);
        PRCI->MTIMECMP[mhart_id] = PRCI->MTIME + g_systick_increment;

        set_csr(mie, MIP_MTIP);

        __enable_irq();

        ret_val = SUCCESS;
    }
    
    return ret_val;
}

/*------------------------------------------------------------------------------
 * RISC-V interrupt handler for machine timer interrupts.
 */
void handle_m_timer_interrupt()
{
    clear_csr(mie, MIP_MTIP);

    SysTick_Handler();

    PRCI->MTIMECMP[read_csr(mhartid)] = PRCI->MTIME + g_systick_increment;

    set_csr(mie, MIP_MTIP);
}

/*------------------------------------------------------------------------------
 * RISC-V interrupt handler for external interrupts.
 */
uint8_t (*ext_irq_handler_table[32])(void) =
{
    Invalid_IRQHandler,
    External_1_IRQHandler,
    External_2_IRQHandler,
    External_3_IRQHandler,
    External_4_IRQHandler,
    External_5_IRQHandler,
    External_6_IRQHandler,
    External_7_IRQHandler,
    External_8_IRQHandler,
    External_9_IRQHandler,
    External_10_IRQHandler,
    External_11_IRQHandler,
    External_12_IRQHandler,
    External_13_IRQHandler,
    External_14_IRQHandler,
    External_15_IRQHandler,
    External_16_IRQHandler,
    External_17_IRQHandler,
    External_18_IRQHandler,
    External_19_IRQHandler,
    External_20_IRQHandler,
    External_21_IRQHandler,
    External_22_IRQHandler,
    External_23_IRQHandler,
    External_24_IRQHandler,
    External_25_IRQHandler,
    External_26_IRQHandler,
    External_27_IRQHandler,
    External_28_IRQHandler,
    External_29_IRQHandler,
    External_30_IRQHandler,
    External_31_IRQHandler
};

/*------------------------------------------------------------------------------
 * 
 */
void handle_m_ext_interrupt()
{
    uint32_t int_num  = PLIC_ClaimIRQ();
    uint8_t disable = EXT_IRQ_KEEP_ENABLED;

    disable = ext_irq_handler_table[int_num]();

    PLIC_CompleteIRQ(int_num);

    if(EXT_IRQ_DISABLE == disable)
    {
    	PLIC_DisableIRQ(int_num);
    }
}

void handle_m_soft_interrupt()
{
    Software_IRQHandler();

    /*Clear software interrupt*/
    PRCI->MSIP[0] = 0x00;
}
/*------------------------------------------------------------------------------
 * Trap/Interrupt handler
 */
#if defined(DETAIL_TRAP_MESSAGES) && defined(MSCC_STDIO_THRU_CORE_UART_APB)

void write_stderr_hex32(uint32_t hex)
{
    uint8_t ii;
    uint8_t jj;
    char towrite;

    write( STDERR_FILENO , "0x", 2 );

    for (ii = 8 ; ii > 0; ii--)
    {
        jj = ii-1;
        uint8_t digit = ((hex & (0xF << (jj*4))) >> (jj*4));
        towrite = digit < 0xA ? ('0' + digit) : ('A' +  (digit - 0xA));
        write( STDERR_FILENO, &towrite, 1);
    }
}

void write_string(const char * str) {
  write(STDERR_FILENO, str, strlen(str));
}

uintptr_t handle_trap(uintptr_t mcause, uintptr_t epc, uintptr_t sp)
{
    if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_EXT))
    {
        handle_m_ext_interrupt();
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_TIMER))
    {
        handle_m_timer_interrupt();
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_SOFT))
    {
        handle_m_soft_interrupt();
    }
    else
    {
        const char * msgDetails  = "\n\nDetails of a trap:";
        const char * msgMcause   = "\nmacuse (exception code): ";
        const char * msgMbadaddr = "\nmbadaddr (mtval):        ";
        const char * msgMtvec    = "\nmtvec (trap vector):     ";
        const char * msgMepc     = "\nmtec (exception pc):     ";
        const char * msgMscratch = "\nmscratch (temporary):    ";
        const char * msgMstatus  = "\nmstatus (status csr):    ";
        const char * msgMip      = "\nmip (interrupt pending): ";
        const char * msgSp       = "\nsp (stack pointer):      ";

        const char * causes[14] = {
            "Instruction address misaligned",
            "Instruction access fault",
            "Illegal instruction",
            "Breakpoint",
            "Load address misaligned",
            "Load address fault",
            "Store access fault",
            "Environment call from U-mode",
            "Environment call from S-mode",
            "Environment call from M-mode",
            "Instruction page fault",
            "Load page fault",
            "Store page fault"
        };

        const char * statuses[23] = {
            "UIE",
            "SIE",
            "invalid",
            "MIE",
            "UPIE",
            "SPIE",
            "invalid",
            "MPIE",
            "SPP",
            "invalid",
            "MPP1",
            "MPP2",
            "FS1",
            "FS2",
            "XS1",
            "XS2",
            "MPRV",
            "SUM",
            "MXR",
            "TVM",
            "TW",
            "TSR"
        };
        // there is SD at XLEN-1 which is not implemented here yet

        const char * msgBadAddrAddr  = "faulting address";
        const char * msgBadAddrInstr = "instruction opcode itself";

        uintptr_t mtvec;
        uintptr_t mscratch;
        uintptr_t mstatus;
        uintptr_t mip;
        uintptr_t mbadaddr;
        asm(
            "csrr %0, mtvec \n\t"
            "csrr %1, mscratch \n\t"
            "csrr %2, mstatus \n\t"
            "csrr %3, mip \n\t"
            "csrr %4, mbadaddr \n\t"
            : "=r" (mtvec), "=r" (mscratch), "=r" (mstatus), "=r" (mip), "=r" (mbadaddr)
        );

        write_string(msgDetails);

        write_string(msgMcause);
        write_stderr_hex32(mcause);
        if (mcause < 14)
        {
          write_string(" ");
          write_string(causes[mcause]);
        }

        write_string(msgMbadaddr);
        write_stderr_hex32(mbadaddr);
        write_string(" ");

        // mbadaddr has differnt meanings depending on the mcause
        switch (mcause) {
          case 0:
          case 4:
          case 6:
            // not sure if these are right, the description is wage and I just guessed
            write_string(msgBadAddrAddr);
            break;

          case 2:
            write_string(msgBadAddrInstr);
            break;

          default:
            break;
        }

        write_string(msgMepc);
        write_stderr_hex32(epc);

        write_string(msgMtvec);
        write_stderr_hex32(mtvec);

        write_string(msgMscratch);
        write_stderr_hex32(mscratch);

        write_string(msgMstatus);
        write_stderr_hex32(mstatus);
        for (int i=0; i < 23; i++)
        {
            if (mstatus & (1 << i))
            {
                write_string(" ");
                write_string(statuses[i]);
            }
        }

        write_string(msgMip);
        write_stderr_hex32(mip);

        write_string(msgSp);
        write_stderr_hex32(sp);

        write_string("\n\r");

        _exit(1 + mcause);
    }
    return epc;
}
#else
uintptr_t handle_trap(uintptr_t mcause, uintptr_t epc)
{
    if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_EXT))
    {
        handle_m_ext_interrupt();
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_TIMER))
    {
        handle_m_timer_interrupt();
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_SOFT))
    {
        handle_m_soft_interrupt();
    }
    else
    {
        _exit(1 + mcause);
    }
    return epc;
}
#endif

#ifdef __cplusplus
}
#endif
