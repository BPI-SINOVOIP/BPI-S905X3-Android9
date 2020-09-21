	/* Sample initialization file */

	.extern APP_Main
	.extern exit

	.text
	.code 32


	.align  0

	.extern __bss_beg__
	.extern __bss_end__
	.extern __stack_end__

	.global start
	.global endless_loop

	/* Stack Sizes */
    .set  UND_STACK_SIZE, 0x00000040
    .set  ABT_STACK_SIZE, 0x00000040
    .set  FIQ_STACK_SIZE, 0x00000400
    .set  IRQ_STACK_SIZE, 0X00000400
    .set  SVC_STACK_SIZE, 0x00000400

	/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs */
    .set  MODE_USR, 0x10            /* User Mode */
    .set  MODE_FIQ, 0x11            /* FIQ Mode */
    .set  MODE_IRQ, 0x12            /* IRQ Mode */
    .set  MODE_SVC, 0x13            /* Supervisor Mode */
    .set  MODE_ABT, 0x17            /* Abort Mode */
    .set  MODE_UND, 0x1B            /* Undefined Mode */
    .set  MODE_SYS, 0x1F            /* System Mode */

    .equ  I_BIT, 0x80               /* when I bit is set, IRQ is disabled */
    .equ  F_BIT, 0x40               /* when F bit is set, FIQ is disabled */


start:
_start:
_mainCRTStartup:

	/* Disable WatchDog*/
	

	/* start timer_us_03 & timer_ms_03 */
	/*
	ldr  r1, =0xffff
	ldr  r3, =0xc0000500
	str  r1, [r3]
	ldr  r3, =0xc0000900
	str  r1, [r3] 
	*/

	/* Setup a stack for each mode - note that this only sets up a usable stack
	for system/user, SWI and IRQ modes.   Also each mode is setup with
	interrupts initially disabled. */
    	ldr   r0, .LC6
    	msr   CPSR_c, #MODE_UND|I_BIT|F_BIT /* Undefined Instruction Mode */
    	mov   sp, r0
    	sub   r0, r0, #UND_STACK_SIZE
    	msr   CPSR_c, #MODE_ABT|I_BIT|F_BIT /* Abort Mode */
    	mov   sp, r0
    	sub   r0, r0, #ABT_STACK_SIZE
    	msr   CPSR_c, #MODE_FIQ|I_BIT|F_BIT /* FIQ Mode */
    	mov   sp, r0
    	sub   r0, r0, #FIQ_STACK_SIZE
    	msr   CPSR_c, #MODE_IRQ|I_BIT|F_BIT /* IRQ Mode */
    	mov   sp, r0
    	sub   r0, r0, #IRQ_STACK_SIZE
    	msr   CPSR_c, #MODE_SVC|I_BIT|F_BIT /* Supervisor Mode */
    	mov   sp, r0
    	sub   r0, r0, #SVC_STACK_SIZE
    	msr   CPSR_c, #MODE_SYS|I_BIT|F_BIT /* System Mode */
    	mov   sp, r0

	/* We want to start in supervisor mode.  Operation will switch to system
	mode when the first task starts. */
	msr   CPSR_c, #MODE_SVC|I_BIT|F_BIT
	

	/* Clear BSS. */

	mov     a2, #0			/* Fill value */
	mov	fp, a2			/* Null frame pointer */
	mov	r7, a2			/* Null frame pointer for Thumb */

	ldr	r1, .LC1		/* Start of memory block */
	ldr	r3, .LC2		/* End of memory block */
	subs	r3, r3, r1      /* Length of block */
	//beq	.end_clear_loop
	b	.end_clear_loop
	mov	r2, #0

.clear_loop:
	strb	r2, [r1], #1
	subs	r3, r3, #1
	bgt	.clear_loop

.end_clear_loop:

	mov	r0, #0          /* no arguments  */
	mov	r1, #0          /* no argv either */

	bl	APP_Main

endless_loop:
	b      endless_loop


	.align 0

	.LC1:
	.word   __bss_beg__
	.LC2:
	.word   __bss_end__
	.LC6:
	.word   __stack_end__


	/* Setup vector table.  Note that undf, pabt, dabt, fiq just execute
	a null loop. */

.section .startup,"ax"
         .code 32
         .align 0

	b     _start			/* reset - _start		*/
	ldr   pc, _undf			/* undefined - _undf		*/
	ldr   pc, _swi			/* SWI - _swi			*/
	ldr   pc, _pabt			/* program abort - _pabt	*/
	ldr   pc, _dabt			/* data abort - _dabt		*/
	nop				/* reserved			*/
	ldr   pc, _irq   		/* IRQ - _irq  			*/
	ldr   pc, _fiq			/* FIQ - _fiq			*/

_undf:  .word __undf                    /* undefined			*/
_swi:   .word vPortYieldProcessor       /* SWI				*/
_pabt:  .word __pabt                    /* program abort		*/
_dabt:  .word __dabt                    /* data abort			*/
_irq:	.word __irq 			/* IRQ				*/
_fiq:   .word __fiq                     /* FIQ				*/

__undf: b     .                         /* undefined			*/
__pabt: b     .                         /* program abort		*/
__dabt: b     .                         /* data abort			*/
__irq:	b     do_irq		    	/* IRQ				*/
__fiq:  b     .                         /* FIQ				*/

