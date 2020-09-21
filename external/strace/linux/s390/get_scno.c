#ifndef ARCH_REGSET
# define ARCH_REGSET s390_regset
#endif

/* Return codes: 1 - ok, 0 - ignore, other - error. */
static int
arch_get_scno(struct tcb *tcp)
{
	tcp->scno = ARCH_REGSET.gprs[2] ?
		    ARCH_REGSET.gprs[2] : ARCH_REGSET.gprs[1];
	return 1;
}
