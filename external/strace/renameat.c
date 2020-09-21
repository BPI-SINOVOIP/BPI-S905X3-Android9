#include "defs.h"

static void
decode_renameat(struct tcb *tcp)
{
	print_dirfd(tcp, tcp->u_arg[0]);
	printpath(tcp, tcp->u_arg[1]);
	tprints(", ");
	print_dirfd(tcp, tcp->u_arg[2]);
	printpath(tcp, tcp->u_arg[3]);
}

SYS_FUNC(renameat)
{
	decode_renameat(tcp);

	return RVAL_DECODED;
}

#include <linux/fs.h>
#include "xlat/rename_flags.h"

SYS_FUNC(renameat2)
{
	decode_renameat(tcp);
	tprints(", ");
	printflags(rename_flags, tcp->u_arg[4], "RENAME_??");

	return RVAL_DECODED;
}
