/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * linux/arch/arm/kernel/entry-v7m.S
 *
 * Copyright (C) 2008 ARM Ltd.
 *
 * Low-level vector interface routines for the ARMv7-M architecture
 */
#include <asm/memory.h>
#include <asm/glue.h>
#include <asm/thread_notify.h>
#include <asm/v7m.h>

#include "entry-header.S"

#ifdef CONFIG_TRACE_IRQFLAGS
#error "CONFIG_TRACE_IRQFLAGS not supported on the current ARMv7M implementation"
#endif

__invalid_entry:
	v7m_exception_entry
#ifdef CONFIG_PRINTK
	adr	r0, strerr
	mrs	r1, ipsr
	mov	r2, lr
	bl	printk
#endif
	mov	r0, sp
	bl	show_regs
1:	b	1b
ENDPROC(__invalid_entry)

strerr:	.asciz	"\nUnhandled exception: IPSR = %08lx LR = %08lx\n"

	.align	2
__irq_entry:
	v7m_exception_entry

	@
	@ Invoke the IRQ handler
	@
	mrs	r0, ipsr
	ldr	r1, =V7M_xPSR_EXCEPTIONNO
	and	r0, r1
	sub	r0, #16
	mov	r1, sp
	stmdb	sp!, {lr}
	@ routine called with r0 = irq number, r1 = struct pt_regs *
	bl	nvic_handle_irq

	pop	{lr}
	@
	@ Check for any pending work if returning to user
	@
	ldr	r1, =BASEADDR_V7M_SCB
	ldr	r0, [r1, V7M_SCB_ICSR]
	tst	r0, V7M_SCB_ICSR_RETTOBASE
	beq	2f

	get_thread_info tsk
	ldr	r2, [tsk, #TI_FLAGS]
	movs	r2, r2, lsl #16
	beq	2f			@ no work pending
	mov	r0, #V7M_SCB_ICSR_PENDSVSET
	str	r0, [r1, V7M_SCB_ICSR]	@ raise PendSV

2:
	@ registers r0-r3 and r12 are automatically restored on exception
	@ return. r4-r7 were not clobbered in v7m_exception_entry so for
	@ correctness they don't need to be restored. So only r8-r11 must be
	@ restored here. The easiest way to do so is to restore r0-r7, too.
	ldmia	sp!, {r0-r11}
	add	sp, #PT_REGS_SIZE-S_IP
	cpsie	i
	bx	lr
ENDPROC(__irq_entry)

__pendsv_entry:
	v7m_exception_entry

	ldr	r1, =BASEADDR_V7M_SCB
	mov	r0, #V7M_SCB_ICSR_PENDSVCLR
	str	r0, [r1, V7M_SCB_ICSR]	@ clear PendSV

	@ execute the pending work, including reschedule
	get_thread_info tsk
	mov	why, #0
	b	ret_to_user_from_irq
ENDPROC(__pendsv_entry)

/*
 * Register switch for ARMv7-M processors.
 * r0 = previous task_struct, r1 = previous thread_info, r2 = next thread_info
 * previous and next are guaranteed not to be the same.
 */
ENTRY(__switch_to)
	.fnstart
	.cantunwind
	add	ip, r1, #TI_CPU_SAVE
#ifdef CONFIG_ARMV7M_ENABLE_FPU
	vstmia  ip!, {s0 - s31}
#endif
	stmia	ip!, {r4 - r11}		@ Store most regs on stack
	str	sp, [ip], #4
	str	lr, [ip], #4
	mov	r5, r0
	add	r4, r2, #TI_CPU_SAVE
	ldr	r0, =thread_notify_head
	mov	r1, #THREAD_NOTIFY_SWITCH
	bl	atomic_notifier_call_chain
	mov	ip, r4
	mov	r0, r5
#ifdef CONFIG_ARMV7M_ENABLE_FPU
	vldmia  ip!, {s0 - s31}
#endif
	ldmia	ip!, {r4 - r11}		@ Load all regs saved previously
	ldr	sp, [ip]
	ldr	pc, [ip, #4]!
	.fnend
ENDPROC(__switch_to)

	.data
#if CONFIG_CPU_V7M_NUM_IRQ <= 112
	.align	9
#else
	.align	10
#endif

/*
 * Vector table (Natural alignment need to be ensured)
 */
ENTRY(vector_table)
	.long	0			@ 0 - Reset stack pointer
	.long	__invalid_entry		@ 1 - Reset
	.long	__invalid_entry		@ 2 - NMI
	.long	__invalid_entry		@ 3 - HardFault
	.long	__invalid_entry		@ 4 - MemManage
	.long	__invalid_entry		@ 5 - BusFault
	.long	__invalid_entry		@ 6 - UsageFault
	.long	__invalid_entry		@ 7 - Reserved
	.long	__invalid_entry		@ 8 - Reserved
	.long	__invalid_entry		@ 9 - Reserved
	.long	__invalid_entry		@ 10 - Reserved
	.long	vector_swi		@ 11 - SVCall
	.long	__invalid_entry		@ 12 - Debug Monitor
	.long	__invalid_entry		@ 13 - Reserved
	.long	__pendsv_entry		@ 14 - PendSV
	.long	__invalid_entry		@ 15 - SysTick
	.rept	CONFIG_CPU_V7M_NUM_IRQ
	.long	__irq_entry		@ External Interrupts
	.endr
	.align	2
	.globl	exc_ret
exc_ret:
	.space	4
