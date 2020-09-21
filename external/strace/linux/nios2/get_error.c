static void
get_error(struct tcb *tcp, const bool check_errno)
{
	/*
	 * The system call convention specifies that r2 contains the return
	 * value on success or a positive error number on failure.  A flag
	 * indicating successful completion is written to r7; r7=0 indicates
	 * the system call success, r7=1 indicates an error.  The positive
	 * errno value written in r2.
	 */
	if (nios2_regs.regs[7]) {
		tcp->u_rval = -1;
		tcp->u_error = nios2_regs.regs[2];
	} else {
		tcp->u_rval = nios2_regs.regs[2];
	}
}
