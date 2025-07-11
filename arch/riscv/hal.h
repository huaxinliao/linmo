#pragma once

#include <types.h>

/* Symbols from the linker script, defining memory boundaries. */
/* From linker script */
extern uint32_t _stack_start, _stack_end; /* Start/end of the STACK memory. */
extern uint32_t _heap_start, _heap_end;   /* Start/end of the HEAP memory. */
extern uint32_t _heap_size;               /* Size of HEAP memory. */
/* Start address for the contents initialization of the .data section. */
extern uint32_t _sidata;
/* Start/end address for the .data section. */
extern uint32_t _sdata, _edata;
/* Start/end address for the .bss section. */
extern uint32_t _sbss, _ebss;
/* Start address of the heap memory. */
extern uint32_t _end;

/* Read a RISC-V Control and Status Register (CSR).
 * @reg : The symbolic name of the CSR (e.g., mstatus).
 */
#define read_csr(reg)                                 \
    ({                                                \
        uint32_t __tmp;                               \
        asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp;                                        \
    })

/* Write a value to a RISC-V CSR.
 * @reg : The symbolic name of the CSR.
 * @val : The 32-bit value to write.
 */
#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })

/* Globally enable or disable machine-level interrupts by setting mstatus.MIE.
 * @enable : Non-zero to enable, zero to disable.
 * Returns the previous state of the interrupt enable bit (1 if enabled, 0 if
 * disabled).
 */
static inline int32_t hal_interrupt_set(int32_t enable)
{
    uint32_t mstatus_val = read_csr(mstatus);
    uint32_t mie_bit = (1U << 3); /* MSTATUS_MIE bit position */

    if (enable) {
        write_csr(mstatus, mstatus_val | mie_bit);
    } else {
        write_csr(mstatus, mstatus_val & ~mie_bit);
    }
    return (int32_t) ((mstatus_val >> 3) & 1);
}

/* Convenience macros for interrupt control */
#define _di() hal_interrupt_set(0) /* Macro to disable global interrupts. */
#define _ei() hal_interrupt_set(1) /* Macro to enable global interrupts. */

/* Jump buffer for saving and restoring a task's execution context.
 * It is structured to hold all callee-saved general-purpose registers as
 * specified by the RISC-V ABI, plus essential pointers and CSRs needed to
 * fully capture a task's state. The exact layout is defined by the CONTEXT_*
 * macros in hal.c.
 *
 * Memory layout (19 x 32-bit words):
 * [0-11]:  s0-s11 (callee-saved registers)
 * [12]:    gp (global pointer)
 * [13]:    tp (thread pointer)
 * [14]:    sp (stack pointer)
 * [15]:    ra (return address)
 * [16]:    mcause (for debugging)
 * [17]:    mepc (exception PC)
 * [18]:    mstatus (status register)
 */
typedef uint32_t jmp_buf[19];

/* Core context-switching primitives. */
int32_t setjmp(jmp_buf env);
void longjmp(jmp_buf env, int32_t val);
void hal_dispatch_init(jmp_buf env);

/* Provides a blocking, busy-wait delay.
 * This function monopolizes the CPU and should only be used for very short
 * delays or in pre-scheduling initialization code.
 * @msec : The number of milliseconds to wait.
 */
void delay_ms(uint32_t msec);

/* Reads the system's high-resolution timer.
 * Returns the number of microseconds since boot.
 */
uint64_t _read_us(void);

/* Hardware Abstraction Layer (HAL) initialization and control functions. */
void hal_hardware_init(void);
void hal_timer_enable(void);
void hal_timer_disable(void);
void hal_interrupt_tick(void);

/* Initializes the context structure for a new task.
 * @ctx : Pointer to jmp_buf to initialize (must be non-NULL).
 * @sp  : Base address of the task's stack (must be valid).
 * @ss  : Total size of the stack in bytes (must be >= MIN_STACK_SIZE).
 * @ra  : The task's entry point function (must be non-NULL).
 */
void hal_context_init(jmp_buf *ctx, size_t sp, size_t ss, size_t ra);

/* Halts the CPU in an unrecoverable error state, shutting down if possible. */
void hal_panic(void);

/* Puts the CPU into a low-power wait-for-interrupt state. */
void hal_cpu_idle(void);

/* Default stack size for new tasks if not otherwise specified. */
#define DEFAULT_STACK_SIZE 4096
