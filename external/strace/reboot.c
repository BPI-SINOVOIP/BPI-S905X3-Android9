#include "defs.h"

#include "xlat/bootflags1.h"
#include "xlat/bootflags2.h"
#include "xlat/bootflags3.h"

SYS_FUNC(reboot)
{
	const unsigned int magic1 = tcp->u_arg[0];
	const unsigned int magic2 = tcp->u_arg[1];
	const unsigned int cmd = tcp->u_arg[2];

	printxval(bootflags1, magic1, "LINUX_REBOOT_MAGIC_???");
	tprints(", ");
	printxval(bootflags2, magic2, "LINUX_REBOOT_MAGIC_???");
	tprints(", ");
	printxval(bootflags3, cmd, "LINUX_REBOOT_CMD_???");
	if (cmd == LINUX_REBOOT_CMD_RESTART2) {
		tprints(", ");
		/*
		 * The size of kernel buffer is 256 bytes and
		 * the last byte is always zero, at most 255 bytes
		 * are copied from the user space.
		 */
		printstr_ex(tcp, tcp->u_arg[3], 255, QUOTE_0_TERMINATED);
	}
	return RVAL_DECODED;
}
