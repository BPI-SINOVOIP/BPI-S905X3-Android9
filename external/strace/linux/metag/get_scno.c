/* Return codes: 1 - ok, 0 - ignore, other - error. */
static int
arch_get_scno(struct tcb *tcp)
{
	tcp->scno = metag_regs.dx[0][1];  /* syscall number in D1Re0 (D1.0) */
	return 1;
}
