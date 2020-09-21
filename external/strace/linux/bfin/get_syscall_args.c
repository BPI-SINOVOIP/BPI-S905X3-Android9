/* Return -1 on error or 1 on success (never 0!). */
static int
get_syscall_args(struct tcb *tcp)
{
	static const int argreg[MAX_ARGS] = {
		PT_R0, PT_R1, PT_R2, PT_R3, PT_R4, PT_R5
	};
	unsigned int i;

	for (i = 0; i < tcp->s_ent->nargs; ++i)
		if (upeek(tcp, argreg[i], &tcp->u_arg[i]) < 0)
			return -1;
	return 1;
}
