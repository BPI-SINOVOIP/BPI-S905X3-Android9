static int
arch_set_error(struct tcb *tcp)
{
	ia64_regs.gr[8] = tcp->u_error;
	ia64_regs.gr[10] = -1;

	return set_regs(tcp->pid);
}

static int
arch_set_success(struct tcb *tcp)
{
	ia64_regs.gr[8] = tcp->u_rval;
	ia64_regs.gr[10] = 0;

	return set_regs(tcp->pid);
}
