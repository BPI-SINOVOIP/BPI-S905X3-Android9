static void
get_error(struct tcb *tcp, const bool check_errno)
{
	if (alpha_a3) {
		tcp->u_rval = -1;
		tcp->u_error = alpha_r0;
	} else {
		tcp->u_rval = alpha_r0;
	}
}
