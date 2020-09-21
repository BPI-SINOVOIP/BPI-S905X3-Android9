#include "defs.h"

static void
decode_chmod(struct tcb *tcp, const int offset)
{
	printpath(tcp, tcp->u_arg[offset]);
	tprints(", ");
	print_numeric_umode_t(tcp->u_arg[offset + 1]);
}

SYS_FUNC(chmod)
{
	decode_chmod(tcp, 0);

	return RVAL_DECODED;
}

SYS_FUNC(fchmodat)
{
	print_dirfd(tcp, tcp->u_arg[0]);
	decode_chmod(tcp, 1);

	return RVAL_DECODED;
}

SYS_FUNC(fchmod)
{
	printfd(tcp, tcp->u_arg[0]);
	tprints(", ");
	print_numeric_umode_t(tcp->u_arg[1]);

	return RVAL_DECODED;
}
