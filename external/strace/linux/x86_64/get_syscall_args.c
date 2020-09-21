/* Return -1 on error or 1 on success (never 0!). */
static int
get_syscall_args(struct tcb *tcp)
{
	if (x86_io.iov_len != sizeof(i386_regs)) {
		/* x86-64 or x32 ABI */
		if (tcp->s_ent->sys_flags & COMPAT_SYSCALL_TYPES) {
			/*
			 * X32 compat syscall: zero-extend from 32 bits.
			 * Use truncate_klong_to_current_wordsize(tcp->u_arg[N])
			 * in syscall handlers
			 * if you need to use *sign-extended* parameter.
			 */
			tcp->u_arg[0] = (uint32_t) x86_64_regs.rdi;
			tcp->u_arg[1] = (uint32_t) x86_64_regs.rsi;
			tcp->u_arg[2] = (uint32_t) x86_64_regs.rdx;
			tcp->u_arg[3] = (uint32_t) x86_64_regs.r10;
			tcp->u_arg[4] = (uint32_t) x86_64_regs.r8;
			tcp->u_arg[5] = (uint32_t) x86_64_regs.r9;
		} else {
			tcp->u_arg[0] = x86_64_regs.rdi;
			tcp->u_arg[1] = x86_64_regs.rsi;
			tcp->u_arg[2] = x86_64_regs.rdx;
			tcp->u_arg[3] = x86_64_regs.r10;
			tcp->u_arg[4] = x86_64_regs.r8;
			tcp->u_arg[5] = x86_64_regs.r9;
		}
	} else {
		/*
		 * i386 ABI: zero-extend from 32 bits.
		 * Use truncate_klong_to_current_wordsize(tcp->u_arg[N])
		 * in syscall handlers
		 * if you need to use *sign-extended* parameter.
		 */
		tcp->u_arg[0] = (uint32_t) i386_regs.ebx;
		tcp->u_arg[1] = (uint32_t) i386_regs.ecx;
		tcp->u_arg[2] = (uint32_t) i386_regs.edx;
		tcp->u_arg[3] = (uint32_t) i386_regs.esi;
		tcp->u_arg[4] = (uint32_t) i386_regs.edi;
		tcp->u_arg[5] = (uint32_t) i386_regs.ebp;
	}
	return 1;
}
