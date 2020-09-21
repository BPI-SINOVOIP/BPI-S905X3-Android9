#include <asm/rse.h>

/* Return -1 on error or 1 on success (never 0!). */
static int
get_syscall_args(struct tcb *tcp)
{
	unsigned long *rbs_end =
		(unsigned long *) ia64_regs.ar[PT_AUR_BSP];
	unsigned long sof = (ia64_regs.cfm >> 0) & 0x7f;
	unsigned long sol = (ia64_regs.cfm >> 7) & 0x7f;
	unsigned long *out0 = ia64_rse_skip_regs(rbs_end, -sof + sol);
	unsigned int i;

	for (i = 0; i < tcp->s_ent->nargs; ++i) {
		if (umove(tcp,
			  (unsigned long) ia64_rse_skip_regs(out0, i),
			  &tcp->u_arg[i]) < 0) {
			if (errno == EPERM)
				tcp->u_arg[i] = 0;
			else
				return -1;
		}
	}

	return 1;
}
