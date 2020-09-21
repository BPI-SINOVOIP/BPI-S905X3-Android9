/*
 * Common code for DHD command-line utility
 *
 * Copyright (C) 1999-2013, Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: dhdu.c 385965 2013-02-19 04:33:34Z $
 */

/* For backwards compatibility, the absence of the define 'BWL_NO_FILESYSTEM_SUPPORT'
 * implies that a filesystem is supported.
 */
#if !defined(BWL_NO_FILESYSTEM_SUPPORT)
#define BWL_FILESYSTEM_SUPPORT
#endif

#ifndef PROP_TXSTATUS
#define PROP_TXSTATUS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>

#include <typedefs.h>
#include <epivers.h>
#include <proto/ethernet.h>
#include <dhdioctl.h>
#include <sdiovar.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include "dhdu.h"
#include "miniopt.h"
#include <proto/bcmip.h>
#include <hndrte_debug.h>
#include <hndrte_armtrap.h>
#include <hndrte_cons.h>
#define IPV4_ADDR_LEN 4

#include <errno.h>

#include <trxhdr.h>
#include "ucode_download.h"

#define stricmp strcasecmp
#define strnicmp strncasecmp


static cmd_func_t dhd_var_void;
static cmd_func_t dhd_varint, dhd_varstr;
static cmd_func_t dhd_var_getandprintstr, dhd_var_getint, dhd_var_get;
static cmd_func_t dhd_var_setint;

static cmd_func_t dhd_version, dhd_list, dhd_msglevel;

#ifdef SDTEST
static cmd_func_t dhd_pktgen;
#endif
static cmd_func_t dhd_sprom;
static cmd_func_t dhd_sdreg;
static cmd_func_t dhd_sd_msglevel, dhd_sd_blocksize, dhd_sd_mode, dhd_sd_reg;
static cmd_func_t dhd_dma_mode;
static cmd_func_t dhd_membytes, dhd_download, dhd_dldn,
	dhd_upload, dhd_coredump, dhd_consoledump, dhd_vars, dhd_idleclock, dhd_idletime;
static cmd_func_t dhd_logstamp;

static cmd_func_t dhd_hostreorder_flows;

#ifdef PROP_TXSTATUS
static cmd_func_t dhd_proptxstatusenable;
static cmd_func_t dhd_proptxstatusmode;
static cmd_func_t dhd_proptxopt;
#endif
static int dhd_var_getbuf(void *dhd, char *iovar, void *param, int param_len, void **bufptr);
static int dhd_var_setbuf(void *dhd, char *iovar, void *param, int param_len);

static uint dhd_iovar_mkbuf(char *name, char *data, uint datalen,
                            char *buf, uint buflen, int *perr);
static int dhd_iovar_getint(void *dhd, char *name, int *var);
static int dhd_iovar_setint(void *dhd, char *name, int var);

#if defined(BWL_FILESYSTEM_SUPPORT)
static int file_size(char *fname);
static int read_vars(char *fname, char *buf, int buf_maxlen);
#endif


/* dword align allocation */
static union {
	char bufdata[DHD_IOCTL_MAXLEN];
	uint32 alignme;
} bufstruct_dhd;
static char *buf = (char*) &bufstruct_dhd.bufdata;

/* integer output format, default to signed integer */
static uint8 int_fmt;

#define DEBUG_INFO_PTRS_END 0xffffffff
const uint32 debug_info_ptrs[] = {0xf8, 0x878, DEBUG_INFO_PTRS_END};

typedef struct {
	uint value;
	char *string;
} dbg_msg_t;

static int dhd_do_msglevel(void *dhd, cmd_t *cmd, char **argv, dbg_msg_t *dbg_msg);

/* Actual command table */
cmd_t dhd_cmds[] = {
	{ "cmds", dhd_list, -1, -1,
	"generate a short list of available commands"},
	{ "version", dhd_version, DHD_GET_VAR, -1,
	"get version information" },
	{ "msglevel", dhd_msglevel, DHD_GET_VAR, DHD_SET_VAR,
	"get/set message bits" },
	{ "bcmerrorstr", dhd_var_getandprintstr, DHD_GET_VAR, -1,
	"errorstring"},
	{ "wdtick", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"watchdog tick time (ms units)"},
	{ "intr", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"use interrupts on the bus"},
	{ "pollrate", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"number of ticks between bus polls (0 means no polling)"},
	{ "idletime", dhd_idletime, DHD_GET_VAR, DHD_SET_VAR,
	"number of ticks for activity timeout (-1: immediate, 0: never)"},
	{ "idleclock", dhd_idleclock, DHD_GET_VAR, DHD_SET_VAR,
	"idleclock active | stopped | <N>\n"
	"\tactive (0)   - do not request any change to the SD clock\n"
	"\tstopped (-1) - request SD clock be stopped on activity timeout\n"
	"\t<N> (other)  - an sd_divisor value to request on activity timeout\n"},
	{ "sd1idle", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"change mode to SD1 when turning off clock at idle"},
	{ "forceeven", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"force SD tx/rx buffers to be even"},
	{ "readahead", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"enable readahead feature (look for next frame len in headers)"},
	{ "sdrxchain", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"enable packet chains to SDIO stack for glom receive"},
	{ "alignctl", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"align control frames"},
	{ "sdalign", dhd_varint, DHD_GET_VAR, -1,
	"display the (compiled in) alignment target for sd requests"},
	{ "txbound", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"get/set maximum number of tx frames per scheduling"},
	{ "rxbound", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"get/set maximum number of rx frames per scheduling"},
	{ "txminmax", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"get/set maximum number of tx frames per scheduling while rx frames outstanding"},
	{ "dconpoll", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"g/set dongle console polling interval (ms)"},
	{ "dump", dhd_varstr, DHD_GET_VAR, -1,
	"dump information"},
	{ "cons", dhd_varstr, -1, DHD_SET_VAR,
	"send string to device console (sd only)"},
	{ "clearcounts", dhd_var_void, -1, DHD_SET_VAR,
	"reset the bus stats shown in the dhd dump"},
	{ "logdump", dhd_varstr, DHD_GET_VAR, -1,
	"dump the timestamp logging buffer"},
	{ "logcal", dhd_varint, -1, DHD_SET_VAR,
	"logcal <n>  -- log around an osl_delay of <n> usecs"},
	{ "logstamp", dhd_logstamp, -1, DHD_SET_VAR,
	"logstamp [<n1>] [<n2>]  -- add a message to the log"},
	{ "ramstart", dhd_varint, DHD_GET_VAR, -1,
	"display start address of onchip SOCRAM"},
	{ "ramsize", dhd_varint, DHD_GET_VAR, -1,
	"display size of onchip SOCRAM"},
	{ "membytes", dhd_membytes, DHD_GET_VAR, DHD_SET_VAR,
	"membytes [-h | -r | -i] <address> <length> [<data>]\n"
	"\tread or write data in the dongle ram\n"
	"\t-h   <data> is a sequence of hex digits rather than a char string\n"
	"\t-r   output binary to stdout rather hex\n"},
	{ "download", dhd_download, -1, DHD_SET_VAR,
	"download [-a <address>] [--noreset] [--norun] [--verify] <binfile> [<varsfile>]\n"
	"\tdownload file to specified dongle ram address and start CPU\n"
	"\toptional vars file will replace vars parsed from the CIS\n"
	"\t--noreset    do not reset SOCRAM core before download\n"
	"\t--norun      do not start dongle CPU after download\n"
	"\t--verify     do readback verify \n"
	"\tdefault <address> is 0\n"},
	{ "dldn", dhd_dldn, -1, DHD_SET_VAR,
	"download <binfile>\n"
	"\tdownload file to specified dongle ram address 0\n"},
	{ "vars", dhd_vars, DHD_GET_VAR, DHD_SET_VAR,
	"vars [<file>]\n"
	"\toverride SPROM vars with <file> (before download)\n"},
	{ "coredump", dhd_coredump, -1, -1,
	"coredump <file>\n"
	"\tdump dongle RAM content into a file in dumpfile format\n"
	"\tfor use with ELF core generator"},
	{ "consoledump", dhd_consoledump, -1, -1,
	"consoledump\n"
	"\tdump dongle debug console buffer"},
	{ "upload", dhd_upload, -1, -1,
	"upload [-a <address> ] <file> [<size>]\n"
	"\tupload dongle RAM content into a file\n"
	"\tdefault <address> is 0, default <size> is RAM size"},
	{ "srdump", dhd_sprom, DHD_GET_VAR, -1,
	"display SPROM content" },
	{ "srwrite", dhd_sprom, -1, DHD_SET_VAR,
	"write data or file content to SPROM\n"
	"\tsrwrite <word-offset> <word-value> ...\n"
	"\tsrwrite [-c] <srom-file-path>\n"
	"\t  -c means write regardless of crc"},
	{ "sleep", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"enter/exit simulated host sleep (bus powerdown w/OOB wakeup)"},
	{ "kso", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"keep sdio on"},
	{ "devcap", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"brcm device capabilities"},
	{ "devsleep", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"Sleep CMD14"},
#ifdef SDTEST
	{ "extloop", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"external loopback: convert all tx data to echo test frames"},
	{ "pktgen", dhd_pktgen, DHD_GET_VAR, DHD_SET_VAR,
	"configure/report pktgen status (SDIO)\n"
	"\t-f N     frequency: send/recv a burst every N ticks\n"
	"\t-c N     count: send/recv N packets each burst\n"
	"\t-t N     total: stop after a total of N packets\n"
	"\t-p N     print: display counts on console every N bursts\n"
	"\t-m N     min: set minimum length of packet data\n"
	"\t-M N     Max: set maximum length of packet data\n"
	"\t-l N     len: set fixed length of packet data\n"
	"\t-s N     stop after N tx failures\n"
	"\t-d dir   test direction/type:\n"
	"\t            send -- send packets discarded by dongle\n"
	"\t            echo -- send packets to be echoed by dongle\n"
	"\t            burst -- request bursts (of size <-c>) from dongle\n"
	"\t              one every <-f> ticks, until <-t> total requests\n"
	"\t            recv -- request dongle enter continuous send mode,\n"
	"\t              read up to <-c> pkts every <-f> ticks until <-t>\n"
	"\t              total reads\n"},
#endif /* SDTEST */
	{ "dngl_isolation", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"g/set dongle isolation, so the dev could be disabled with out effecting the dongle state"},
	{ "sdreg", dhd_sdreg, DHD_GET_VAR, DHD_SET_VAR,
	"g/set sdpcmdev core register (f1) across SDIO (CMD53)"},
	{ "sbreg", dhd_sdreg, DHD_GET_VAR, DHD_SET_VAR,
	"g/set any backplane core register (f1) across SDIO (CMD53)"},
	{ "sd_cis", dhd_var_getandprintstr, DHD_GET_VAR, -1,
	"dump sdio CIS"},
	{ "sd_devreg", dhd_sd_reg, DHD_GET_VAR, DHD_SET_VAR,
	"g/set device register across SDIO bus (CMD52)"},
	{ "sd_hostreg", dhd_sd_reg, DHD_GET_VAR, DHD_SET_VAR,
	"g/set local controller register"},
	{ "sd_blocksize", dhd_sd_blocksize, DHD_GET_VAR, DHD_SET_VAR,
	"g/set block size for a function"},
	{ "sd_blockmode", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"g/set blockmode"},
	{ "sd_ints", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"g/set client ints"},
	{ "sd_dma", dhd_dma_mode, DHD_GET_VAR, DHD_SET_VAR,
	"g/set dma usage: [PIO | SDMA | ADMA1 | ADMA2]"},
	{ "sd_yieldcpu", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"allow blocking (yield of CPU) on data xfer"},
	{ "sd_minyield", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"minimum xfer size to allow CPU yield"},
	{ "sd_forcerb", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"force readback when changing local interrupt settings"},
	{ "sd_numints", dhd_varint, DHD_GET_VAR, -1,
	"number of device interrupts"},
	{ "sd_numlocalints", dhd_varint, DHD_GET_VAR, -1,
	"number of non-device interrupts"},
	{ "sd_divisor", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"set the divisor for SDIO clock generation"},
	{ "sd_power", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"set the SD Card slot power"},
	{ "sd_power_save", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"set the SDIO3.0 power save value"},
	{ "sd_clock", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"turn on/off the SD Clock"},
	{ "sd_crc", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"turn on/off CRC checking in SPI mode"},
	{ "sd_mode", dhd_sd_mode, DHD_GET_VAR, DHD_SET_VAR,
	"g/set SDIO bus mode (spi, sd1, sd4)"},
	{ "sd_highspeed", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"set the high-speed clocking mode"},
	{ "sd_msglevel", dhd_sd_msglevel, DHD_GET_VAR, DHD_SET_VAR,
	"g/set debug message level"},
	{ "sd_hciregs", dhd_varstr, DHD_GET_VAR, -1,
	"display host-controller interrupt registers"},
	{ "sdiod_drive", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"SDIO Device drive strength in milliamps. (0=tri-state, 1-12mA)"},
	{ "devreset", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"Move device into or out of reset state (1/reset, or 0/operational)"},
	{ "ioctl_timeout", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"IOCTL response timeout (milliseconds)."},
#ifdef PROP_TXSTATUS
	{ "proptx", dhd_proptxstatusenable, DHD_GET_VAR, DHD_SET_VAR,
	"enable/disable the proptxtstatus feature\n"
	"0 - disabled\n"
	"1 - enabled\n"},
	{ "ptxmode", dhd_proptxstatusmode, DHD_GET_VAR, DHD_SET_VAR,
	"set the proptxtstatus operation mode:\n"
	"0 - Unsupported\n"
	"1 - Use implied credit from a packet status\n"
	"2 - Use explicit credit\n" },
	{ "proptx_opt", dhd_proptxopt, DHD_GET_VAR, DHD_SET_VAR,
	"enable/disable proptxtstatus optimizations to increase throughput:\n"
	"0 - Unsupported\n"
	"1 - Enable proptxstatus optimizations to increase throughput\n" },
#endif
	{ "sd_uhsimode", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"g/set UHSI Mode"},
	{ "host_reorder_flows", dhd_hostreorder_flows, DHD_GET_VAR, -1,
	"get host reorder flows "},
	{ "txglomsize", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"max glom size for sdio tx\n"},
	{ "txglommode", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"glom mode for sdio tx 0- copy, 1- multidescriptor\n"},
	{ "fw_hang_report", dhd_varint, DHD_GET_VAR, DHD_SET_VAR,
	"enable/disable report firmware hangs for firmware reload\n"
	"0 - disabled (for testing)\n"
	"1 - enabled (default)\n"},
	{ NULL, NULL, 0, 0, NULL }
};

cmd_t dhd_varcmd = {"var", dhd_varint, -1, -1, "unrecognized name, type -h for help"};
char *dhdu_av0;

#if defined(BWL_FILESYSTEM_SUPPORT)
static int
file_size(char *fname)
{
	FILE *fp;
	long size = -1;

	/* Can't use stat() because of Win CE */

	if ((fp = fopen(fname, "rb")) == NULL ||
	    fseek(fp, 0, SEEK_END) < 0 ||
	    (size = ftell(fp)) < 0)
		fprintf(stderr, "Could not determine size of %s: %s\n",
		        fname, strerror(errno));

	if (fp != NULL)
		fclose(fp);

	return (int)size;
}
#endif   /* BWL_FILESYSTEM_SUPPORT */


/* parse/validate the command line arguments */
/*
* pargv is updated upon return if the first argument is an option.
 * It remains intact otherwise.
 */
int
dhd_option(char ***pargv, char **pifname, int *phelp)
{
	char *ifname = NULL;
	int help = FALSE;
	int status = CMD_OPT;
	char **argv = *pargv;

	int_fmt = INT_FMT_DEC;

	while (*argv) {
		/* select different adapter */
		if (!strcmp(*argv, "-a") || !strcmp(*argv, "-i")) {
			char *opt = *argv++;
			ifname = *argv;
			if (!ifname) {
				fprintf(stderr,
					"error: expected interface name after option %s\n", opt);
				status = CMD_ERR;
				break;
			}
		}

		/* integer output format */
		else if (!strcmp(*argv, "-d"))
			int_fmt = INT_FMT_DEC;
		else if (!strcmp(*argv, "-u"))
			int_fmt = INT_FMT_UINT;
		else if (!strcmp(*argv, "-x"))
			int_fmt = INT_FMT_HEX;

		/* command usage */
		else if (!strcmp(*argv, "-h"))
			help = TRUE;

		/* done with generic options */
		else {
			status = CMD_DHD;
			break;
		}

		/* consume the argument */
		argv ++;
		break;
	}

	*phelp = help;
	*pifname = ifname;
	*pargv = argv;

	return status;
}

void
dhd_cmd_usage(cmd_t *cmd)
{
	if (strlen(cmd->name) >= 8)
		fprintf(stderr, "%s\n\t%s\n\n", cmd->name, cmd->help);
	else
		fprintf(stderr, "%s\t%s\n\n", cmd->name, cmd->help);
}

/* Dump out short list of commands */
static int
dhd_list(void *dhd, cmd_t *garb, char **argv)
{
	cmd_t *cmd;
	int nrows, i, len;
	char *buf;
	int letter, col, row, pad;

	UNUSED_PARAMETER(dhd);
	UNUSED_PARAMETER(garb);
	UNUSED_PARAMETER(argv);

	for (cmd = dhd_cmds, nrows = 0; cmd->name; cmd++)
		    nrows++;

	nrows /= 4;
	nrows++;

	len = nrows * 80 + 2;
	buf = malloc(len);
	if (buf == NULL) {
		fprintf(stderr, "Failed to allocate buffer of %d bytes\n", len);
		return BCME_NOMEM;
	}
	for (i = 0; i < len; i++)
		*(buf+i) = 0;

	row = col = 0;
	for (letter = 'a'; letter < 'z'; letter++) {
		for (cmd = dhd_cmds; cmd->name; cmd++) {
			if (cmd->name[0] == letter || cmd->name[0] == letter - 0x20) {
				strcat(buf+row*80, cmd->name);
				pad = 18 * (col + 1) - strlen(buf+row*80);
				if (pad < 1)
					pad = 1;
				for (; pad; pad--)
					strcat(buf+row*80, " ");
				row++;
				if (row == nrows) {
					col++; row = 0;
				}
			}
		}
	}
	for (row = 0; row < nrows; row++)
		printf("%s\n", buf+row*80);

	printf("\n");
	free(buf);
	return (0);
}

void
dhd_cmds_usage(cmd_t *port_cmds)
{
	cmd_t *port_cmd;
	cmd_t *cmd;

	/* print usage of port commands */
	for (port_cmd = port_cmds; port_cmd && port_cmd->name; port_cmd++)
		/* Check for wc_cmd */
		dhd_cmd_usage(port_cmd);

	/* print usage of common commands without port counterparts */
	for (cmd = dhd_cmds; cmd->name; cmd++) {
		/* search if port counterpart exists */
		for (port_cmd = port_cmds; port_cmd && port_cmd->name; port_cmd++)
			if (!strcmp(port_cmd->name, cmd->name))
				break;
		if (!port_cmd || !port_cmd->name)
			dhd_cmd_usage(cmd);
	}
}

void
dhd_usage(cmd_t *port_cmds)
{
	fprintf(stderr,
	        "Usage: %s [-a|i <adapter>] [-h] [-d|u|x] <command> [arguments]\n",
		dhdu_av0);

	fprintf(stderr, "\n");
	fprintf(stderr, "  -h		this message\n");
	fprintf(stderr, "  -a, -i	adapter name or number\n");
	fprintf(stderr, "  -d		display values as signed integer\n");
	fprintf(stderr, "  -u		display values as unsigned integer\n");
	fprintf(stderr, "  -x		display values as hexdecimal\n");
	fprintf(stderr, "\n");

	dhd_cmds_usage(port_cmds);
}

int
dhd_check(void *dhd)
{
	int ret;
	int val;

	if ((ret = dhd_get(dhd, DHD_GET_MAGIC, &val, sizeof(int))) < 0)
		return ret;
	if (val != DHD_IOCTL_MAGIC)
		return -1;
	if ((ret = dhd_get(dhd, DHD_GET_VERSION, &val, sizeof(int))) < 0)
		return ret;
	if (val > DHD_IOCTL_VERSION) {
		fprintf(stderr, "Version mismatch, please upgrade\n");
		return -1;
	}
	return 0;
}

void
dhd_printint(int val)
{
	switch (int_fmt) {
	case INT_FMT_UINT:
		printf("%u\n", val);
		break;
	case INT_FMT_HEX:
		printf("0x%x\n", val);
		break;
	case INT_FMT_DEC:
	default:
		printf("%d\n", val);
		break;
	}
}

/* pretty hex print a contiguous buffer (tweaked from wlu) */
void
dhd_hexdump(uchar *buf, uint nbytes, uint saddr)
{
	char line[256];
	char* p;
	uint i;

	if (nbytes == 0) {
		printf("\n");
		return;
	}

	p = line;
	for (i = 0; i < nbytes; i++) {
		if (i % 16 == 0) {
			p += sprintf(p, "%08x: ", saddr + i);	/* line prefix */
		}
		p += sprintf(p, "%02x ", buf[i]);
		if (i % 16 == 15) {
			uint j;
			p += sprintf(p, "  ");
			for (j = i-15; j <= i; j++)
				p += sprintf(p, "%c",
				             ((buf[j] >= 0x20 && buf[j] <= 0x7f) ? buf[j] : '.'));
			printf("%s\n", line);		/* flush line */
			p = line;
		}
	}

	/* flush last partial line */
	if (p != line)
		printf("%s\n", line);
}


#ifdef SDTEST
static int
dhd_pktgen(void *dhd, cmd_t *cmd, char **argv)
{
	int ret = 0;
	void *ptr = NULL;
	dhd_pktgen_t pktgen;
	char *str;

	UNUSED_PARAMETER(dhd);
	UNUSED_PARAMETER(cmd);

	/* Get current settings */
	if ((ret = dhd_var_getbuf(dhd, "pktgen", NULL, 0, &ptr)) != 0)
		return ret;
	memcpy(&pktgen, ptr, sizeof(pktgen));

	if (pktgen.version != DHD_PKTGEN_VERSION) {
		fprintf(stderr, "pktgen version mismatch (module %d app %d)\n",
		        pktgen.version, DHD_PKTGEN_VERSION);
		return BCME_ERROR;
	}

	/* Presence of args implies a set, else a get */
	if (*++argv) {
		miniopt_t opts;
		int opt_err;

		/* Initialize option parser */
		miniopt_init(&opts, "pktgen", "", FALSE);

		while ((opt_err = miniopt(&opts, argv)) != -1) {
			if (opt_err == 1) {
				fprintf(stderr, "pktgen options error\n");
				ret = -1;
				goto exit;
			}
			argv += opts.consumed;

			if (!opts.good_int && opts.opt != 'd') {
				fprintf(stderr, "invalid integer %s\n", opts.valstr);
				ret = -1;
				goto exit;
			}

			switch (opts.opt) {
			case 'f':
				pktgen.freq = opts.uval;
				break;
			case 'c':
				pktgen.count = opts.uval;
				break;
			case 'p':
				pktgen.print = opts.uval;
				break;
			case 't':
				pktgen.total = opts.uval;
				break;
			case 's':
				pktgen.stop = opts.uval;
				break;
			case 'm':
				pktgen.minlen = opts.uval;
				break;
			case 'M':
				pktgen.maxlen = opts.uval;
				break;
			case 'l': case 'L':
				pktgen.minlen = pktgen.maxlen = opts.uval;
				break;
			case 'd':
				if (!strcmp(opts.valstr, "send"))
					pktgen.mode = DHD_PKTGEN_SEND;
				else if (!strcmp(opts.valstr, "echo"))
					pktgen.mode = DHD_PKTGEN_ECHO;
				else if (!strcmp(opts.valstr, "burst"))
					pktgen.mode = DHD_PKTGEN_RXBURST;
				else if (!strcmp(opts.valstr, "recv"))
					pktgen.mode = DHD_PKTGEN_RECV;
				else {
					fprintf(stderr, "unrecognized dir mode %s\n",
					        opts.valstr);
					return BCME_USAGE_ERROR;
				}
				break;

			default:
				fprintf(stderr, "option parsing error (key %s valstr %s)\n",
				        opts.key, opts.valstr);
				ret = BCME_USAGE_ERROR;
				goto exit;
			}
		}

		if (pktgen.maxlen < pktgen.minlen) {
			fprintf(stderr, "min/max error (%d/%d)\n", pktgen.minlen, pktgen.maxlen);
			ret = -1;
			goto exit;
		}

		/* Set the new values */
		ret = dhd_var_setbuf(dhd, "pktgen", &pktgen, sizeof(pktgen));
	} else {
		printf("Counts: %d send attempts, %d received, %d tx failures\n",
		       pktgen.numsent, pktgen.numrcvd, pktgen.numfail);
	}

	/* Show configuration in either case */
	switch (pktgen.mode) {
	case DHD_PKTGEN_ECHO: str = "echo"; break;
	case DHD_PKTGEN_SEND: str = "send"; break;
	case DHD_PKTGEN_RECV: str = "recv"; break;
	case DHD_PKTGEN_RXBURST: str = "burst"; break;
	default: str = "UNKNOWN"; break;
	}

	printf("Config: mode %s %d pkts (len %d-%d) each %d ticks\n",
	       str, pktgen.count, pktgen.minlen, pktgen.maxlen, pktgen.freq);

	/* Second config line for optional items */
	str = "        ";
	if (pktgen.total) {
		printf("%slimit %d", str, pktgen.total);
		str = ", ";
	}
	if (pktgen.print) {
		printf("%sprint every %d ticks", str, (pktgen.freq * pktgen.print));
		str = ", ";
	}
	if (pktgen.stop) {
		printf("%sstop after %d tx failures", str, pktgen.stop);
		str = ", ";
	}
	if (str[0] == ',')
		printf("\n");

exit:
	return ret;
}
#endif /* SDTEST */

static dbg_msg_t dhd_sd_msgs[] = {
	{SDH_ERROR_VAL,	"error"},
	{SDH_TRACE_VAL,	"trace"},
	{SDH_INFO_VAL,	"info"},
	{SDH_DATA_VAL,	"data"},
	{SDH_CTRL_VAL,	"control"},
	{SDH_LOG_VAL,	"log"},
	{SDH_DMA_VAL,	"dma"},
	{0,		NULL}
};

static int
dhd_sd_msglevel(void *dhd, cmd_t *cmd, char **argv)
{
	return dhd_do_msglevel(dhd, cmd, argv, dhd_sd_msgs);
}

static int
dhd_sd_blocksize(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	int argc;
	char *endptr = NULL;
	void *ptr = NULL;
	int func, size;

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	if (argc < 1 || argc > 2) {
		printf("required args: function [size] (size 0 means max)\n");
		return BCME_USAGE_ERROR;
	}

	func = strtol(argv[1], &endptr, 0);
	if (*endptr != '\0') {
		printf("Invalid function: %s\n", argv[1]);
		return BCME_USAGE_ERROR;
	}

	if (argc > 1) {
		size = strtol(argv[2], &endptr, 0);
		if (*endptr != '\0') {
			printf("Invalid size: %s\n", argv[1]);
			return BCME_USAGE_ERROR;
		}
	}

	if (argc == 1) {
		if ((ret = dhd_var_getbuf(dhd, cmd->name, &func, sizeof(func), &ptr)) >= 0)
			printf("Function %d block size: %d\n", func, *(int*)ptr);
	} else {
		printf("Setting function %d block size to %d\n", func, size);
		size &= 0x0000ffff; size |= (func << 16);
		ret = dhd_var_setbuf(dhd, cmd->name, &size, sizeof(size));
	}

	return (ret);
}

static int
dhd_sd_mode(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int argc;
	int sdmode;

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	if (argv[1]) {
		if (!strcmp(argv[1], "spi")) {
			strcpy(argv[1], "0");
		} else if (!strcmp(argv[1], "sd1")) {
			strcpy(argv[1], "1");
		} else if (!strcmp(argv[1], "sd4")) {
			strcpy(argv[1], "2");
		} else {
			return BCME_USAGE_ERROR;
		}

		ret = dhd_var_setint(wl, cmd, argv);

	} else {
		if ((ret = dhd_var_get(wl, cmd, argv))) {
			return (ret);
		} else {
			sdmode = *(int32*)buf;

			printf("SD Mode is: %s\n",
			       sdmode == 0 ? "SPI"
			       : sdmode == 1 ? "SD1"
				   : sdmode == 2 ? "SD4" : "Unknown");
		}
	}

	return (ret);
}

static int
dhd_dma_mode(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int argc;
	int dmamode;

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	if (argv[1]) {
		if (!stricmp(argv[1], "pio")) {
			strcpy(argv[1], "0");
		} else if (!strcmp(argv[1], "0")) {
		} else if (!stricmp(argv[1], "dma")) {
			strcpy(argv[1], "1");
		} else if (!stricmp(argv[1], "sdma")) {
			strcpy(argv[1], "1");
		} else if (!strcmp(argv[1], "1")) {
		} else if (!stricmp(argv[1], "adma1")) {
			strcpy(argv[1], "2");
		} else if (!stricmp(argv[1], "adma")) {
			strcpy(argv[1], "3");
		} else if (!stricmp(argv[1], "adma2")) {
			strcpy(argv[1], "3");
		} else {
			return BCME_USAGE_ERROR;
		}

		ret = dhd_var_setint(wl, cmd, argv);

	} else {
		if ((ret = dhd_var_get(wl, cmd, argv))) {
			return (ret);
		} else {
			dmamode = *(int32*)buf;

			printf("DMA Mode is: %s\n",
			       dmamode == 0 ? "PIO"
			       : dmamode == 1 ? "SDMA"
			       : dmamode == 2 ? "ADMA1"
			       : dmamode == 3 ? "ADMA2"
			       : "Unknown");
		}
	}

	return (ret);
}


static int
dhd_sdreg(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	sdreg_t sdreg;
	uint argc;
	char *ptr = NULL;

	UNUSED_PARAMETER(cmd);

	bzero(&sdreg, sizeof(sdreg));

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	/* required args: offset (will default size) */
	if (argc < 1) {
		printf("required args: offset[/size] [value]\n");
		return BCME_USAGE_ERROR;
	}

	sdreg.offset = strtoul(argv[1], &ptr, 0);
	if (*ptr && *ptr != '/') {
		printf("Bad arg: %s\n", argv[1]);
		return BCME_USAGE_ERROR;
	}

	/* read optional /size */
	if (*ptr == '/') {
		sdreg.func = strtol((ptr+1), &ptr, 0);
		if (*ptr || ((sdreg.func != 2) && sdreg.func != 4)) {
			printf("Bad size option?\n");
			return BCME_USAGE_ERROR;
		}
	}
	else {
		sdreg.func = 4;
		printf("Defaulting to register size 4\n");
	}

	if (argc > 1) {
		sdreg.value = strtoul(argv[2], &ptr, 0);
		if (*ptr) {
			printf("Bad value: %s\n", argv[2]);
			return BCME_USAGE_ERROR;
		}
	}

	if (argc <= 1) {
		ret = dhd_var_getbuf(dhd, argv[0], &sdreg, sizeof(sdreg), (void**)&ptr);
		if (ret >= 0)
			printf("0x%0*x\n", (2 * sdreg.func), *(int *)ptr);
	} else {
		ret = dhd_var_setbuf(dhd, argv[0], &sdreg, sizeof(sdreg));
	}

	return (ret);
}

static int
dhd_membytes(void *dhd, cmd_t *cmd, char **argv)
{
	int ret = -1;
	uint argc;
	char *ptr;
	int params[2];
	uint addr;
	uint len;
	int align;

	int rawout, hexin;

	miniopt_t opts;
	int opt_err;

	/* Parse command-line options */
	miniopt_init(&opts, "membytes", "rh", FALSE);

	rawout = hexin = 0;

	argv++;
	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "membytes options error\n");
			ret = -1;
			goto exit;
		}

		if (opts.positional)
			break;

		argv += opts.consumed;

		if (opts.opt == 'h') {
			hexin = 1;
		} else if (opts.opt == 'r') {
			rawout = 1;
		} else {
			fprintf(stderr, "membytes command error\n");
			ret = -1;
			goto exit;
		}
	}

	/* arg count */
	for (argc = 0; argv[argc]; argc++);

	/* required args: address size [<data>]] */
	if (argc < 2) {
		fprintf(stderr, "required args: address size [<data>]\n");
		return BCME_USAGE_ERROR;
	}

	if (argc < 3 && hexin) {
		fprintf(stderr, "missing <data> required by -h\n");
		return BCME_USAGE_ERROR;
	}
	if ((argc > 2) && (rawout)) {
		fprintf(stderr, "can't have <data> arg with -r\n");
		return BCME_USAGE_ERROR;
	}

	/* read address */
	addr = strtoul(argv[0], &ptr, 0);
	if (*ptr) {
		fprintf(stderr, "Bad arg: %s\n", argv[0]);
		return BCME_USAGE_ERROR;
	}

	/* read size */
	len = strtoul(argv[1], &ptr, 0);
	if (*ptr) {
		fprintf(stderr, "Bad value: %s\n", argv[1]);
		return BCME_USAGE_ERROR;
	}

	align = addr & 0x03;
	if (align && argc > 2) {
		fprintf(stderr, "Can only write starting at long-aligned addresses.\n");
		return BCME_USAGE_ERROR;
	}

	/* get can just use utility function, set must copy custom buffer */
	if (argc == 2) {
		/* Read */
		uint chunk = DHD_IOCTL_MAXLEN;
		for (addr -= align, len += align; len; addr += chunk, len -= chunk, align = 0) {
			chunk = MIN(chunk, len);
			params[0] = addr;
			params[1] = ROUNDUP(chunk, 4);
			ret = dhd_var_getbuf(dhd, "membytes",
			                     params, (2 * sizeof(int)), (void**)&ptr);
			if (ret < 0)
				goto exit;

			if (rawout) {
				fwrite(ptr + align, sizeof(char), chunk - align, stdout);
			} else {
				dhd_hexdump((uchar*)ptr + align, chunk - align, addr + align);
			}
		}
	} else {
		/* Write */
		uint patlen = strlen(argv[2]);
		uint chunk, maxchunk;
		char *sptr;

		if (hexin) {
			char *inptr, *outptr;
			if (patlen & 1) {
				fprintf(stderr, "Hex (-h) must consist of whole bytes\n");
				ret = BCME_USAGE_ERROR;
				goto exit;
			}

			for (inptr = outptr = argv[2]; patlen; patlen -= 2) {
				int n1, n2;

				n1 = (int)((unsigned char)*inptr++);
				n2 = (int)((unsigned char)*inptr++);
				if (!isxdigit(n1) || !isxdigit(n2)) {
					fprintf(stderr, "invalid hex digit %c\n",
					        (isxdigit(n1) ? n2 : n1));
					ret = BCME_USAGE_ERROR;
					goto exit;
				}
				n1 = isdigit(n1) ? (n1 - '0')
				        : ((islower(n1) ? (toupper(n1)) : n1) - 'A' + 10);
				n2 = isdigit(n2) ? (n2 - '0')
				        : ((islower(n2) ? (toupper(n2)) : n2) - 'A' + 10);
				*outptr++ = (n1 * 16) + n2;
			}

			patlen = outptr - argv[2];
		}

		sptr = argv[2];
		maxchunk = DHD_IOCTL_MAXLEN - (strlen(cmd->name) + 1 + (2 * sizeof(int)));

		while (len) {
			chunk = (len > maxchunk) ? (maxchunk & ~0x3) : len;

			/* build the iovar command */
			memset(buf, 0, DHD_IOCTL_MAXLEN);
			strcpy(buf, cmd->name);
			ptr = buf + strlen(buf) + 1;
			params[0] = addr; params[1] = chunk;
			memcpy(ptr, params, (2 * sizeof(int)));
			ptr += (2 * sizeof(int));
			addr += chunk; len -= chunk;

			while (chunk--) {
				*ptr++ = *sptr++;
				if (sptr >= (argv[2] + patlen))
					sptr = argv[2];
			}

			ret = dhd_set(dhd, DHD_SET_VAR, &buf[0], (ptr - buf));
			if (ret < 0)
				goto exit;
		}
	}

exit:
	return ret;
}

static int
dhd_idletime(void *dhd, cmd_t *cmd, char **argv)
{
	int32 idletime;
	char *endptr = NULL;
	int err = 0;

	if (argv[1]) {
		if (!strcmp(argv[1], "never")) {
			idletime = 0;
		} else if (!strcmp(argv[1], "immediate") || !strcmp(argv[1], "immed")) {
			idletime = DHD_IDLE_IMMEDIATE;
		} else {
			idletime = strtol(argv[1], &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid number %s\n", argv[1]);
				err = BCME_USAGE_ERROR;
			}
		}
		if ((idletime < 0) && (idletime != DHD_IDLE_IMMEDIATE)) {
			fprintf(stderr, "invalid value %s\n", argv[1]);
			err = -1;
		}

		if (!err) {
			strcpy(buf, "idletime");
			endptr = buf + strlen(buf) + 1;
			memcpy(endptr, &idletime, sizeof(uint32));
			endptr += sizeof(uint32);
			err = dhd_set(dhd, DHD_SET_VAR, &buf[0], (endptr - buf));
		}
	} else {
		if ((err = dhd_var_get(dhd, cmd, argv))) {
			return err;
		} else {
			idletime = *(int32*)buf;

			if (idletime == 0) {
				printf("0 (never)\n");
			} else if (idletime == DHD_IDLE_IMMEDIATE) {
				printf("-1 (immediate)\n");
			} else if (idletime > 0) {
				printf("%d\n", idletime);
			} else printf("%d (invalid)\n", idletime);
		}
	}
	return err;
}

static int
dhd_idleclock(void *dhd, cmd_t *cmd, char **argv)
{
	int32 idleclock;
	char *endptr = NULL;
	int err = 0;

	if (argv[1]) {
		if (!strcmp(argv[1], "active")) {
			idleclock = DHD_IDLE_ACTIVE;
		} else if (!strcmp(argv[1], "stopped")) {
			idleclock = DHD_IDLE_STOP;
		} else {
			idleclock = strtol(argv[1], &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid number %s\n", argv[1]);
				err = BCME_USAGE_ERROR;
			}
		}

		if (!err) {
			strcpy(buf, "idleclock");
			endptr = buf + strlen(buf) + 1;
			memcpy(endptr, &idleclock, sizeof(int32));
			endptr += sizeof(int32);
			err = dhd_set(dhd, DHD_SET_VAR, &buf[0], (endptr - buf));
		}
	} else {
		if ((err = dhd_var_get(dhd, cmd, argv))) {
			return err;
		} else {
			idleclock = *(int32*)buf;

			if (idleclock == DHD_IDLE_ACTIVE)
				printf("Idleclock %d (active)\n", idleclock);
			else if (idleclock == DHD_IDLE_STOP)
				printf("Idleclock %d (stopped)\n", idleclock);
			else
				printf("Idleclock divisor %d\n", idleclock);
		}
	}
	return err;
}

/* Word count for a 4kb SPROM */
#define SPROM_WORDS 256

static int
dhd_sprom(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	int ret, i;
	uint argc;
	char *endptr;
	char *bufp, *countptr;
	uint16 *wordptr;
	uint offset, words, bytes;
	bool nocrc = FALSE;

	char *fname;
	FILE *fp;

	UNUSED_PARAMETER(cmd);

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	/* init buffer */
	bufp = buf;
	memset(bufp, 0, DHD_IOCTL_MAXLEN);
	strcpy(bufp, "sprom");
	bufp += strlen("sprom") + 1;

	if (strcmp(argv[0], "srdump") == 0) {
		if (argc) {
			fprintf(stderr, "Command srdump doesn't take args\n");
			return BCME_USAGE_ERROR;
		}
		offset = 0;
		words = SPROM_WORDS;
		bytes = 2 * words;

		memcpy(bufp, &offset, sizeof(int));
		bufp += sizeof(int);
		memcpy(bufp, &bytes, sizeof(int));
		bufp += sizeof(int);

		if (!ISALIGNED((uintptr)bufp, sizeof(uint16))) {
			fprintf(stderr, "Internal error: unaligned word buffer\n");
			return BCME_ERROR;
		}
	} else {
		if (strcmp(argv[0], "srwrite") != 0) {
			fprintf(stderr, "Unimplemented sprom command: %s\n", argv[0]);
			return BCME_USAGE_ERROR;
		}

		if (argc == 0) {
			return BCME_USAGE_ERROR;
		} else if ((argc == 1) ||
		           ((argc == 2) && ((nocrc = !strcmp(argv[1], "-c"))))) {

			fname = nocrc ? argv[2] : argv[1];

			/* determine and validate file size */
			if ((ret = file_size(fname)) < 0)
				return BCME_ERROR;

			bytes = ret;
			offset = 0;
			words = bytes / 2;

			if (bytes != 2 * SPROM_WORDS) {
				fprintf(stderr, "Bad file size\n");
				return BCME_ERROR;
			}

			memcpy(bufp, &offset, sizeof(int));
			bufp += sizeof(int);
			memcpy(bufp, &bytes, sizeof(int));
			bufp += sizeof(int);

			if (!ISALIGNED((uintptr)bufp, sizeof(uint16))) {
				fprintf(stderr, "Internal error: unaligned word buffer\n");
				return BCME_ERROR;
			}

			if ((fp = fopen(fname, "rb")) == NULL) {
				fprintf(stderr, "Could not open %s: %s\n",
				        fname, strerror(errno));
				return BCME_ERROR;
			}

			if (fread((uint16*)bufp, sizeof(uint16), words, fp) != words) {
				fprintf(stderr, "Could not read %d bytes from %s\n",
				        words * 2, fname);
				fclose(fp);
				return BCME_ERROR;
			}

			fclose(fp);

			if (!nocrc &&
			    hndcrc8((uint8*)bufp, bytes, CRC8_INIT_VALUE) != CRC8_GOOD_VALUE) {
				fprintf(stderr, "CRC check failed: 0x%02x, should be 0x%02x.\n",
				        ((uint8*)bufp)[bytes-1],
				        ~hndcrc8((uint8*)bufp, bytes - 1, CRC8_INIT_VALUE) & 0xff);
				return BCME_ERROR;
			}

			ltoh16_buf(bufp, bytes);
		} else {
			offset = strtoul(*++argv, &endptr, 0) * 2;
			if (*endptr != '\0') {
				fprintf(stderr, "offset %s is not an integer\n", *argv);
				return BCME_USAGE_ERROR;
			}

			memcpy(bufp, &offset, sizeof(int));
			bufp += sizeof(int);
			countptr = bufp;
			bufp += sizeof(int);

			if (!ISALIGNED((uintptr)bufp, sizeof(uint16))) {
				fprintf(stderr, "Internal error: unaligned word buffer\n");
				return BCME_ERROR;
			}

			for (words = 0, wordptr = (uint16*)bufp; *++argv; words++) {
				*wordptr++ = (uint16)strtoul(*argv, &endptr, 0);
				if (*endptr != '\0') {
					fprintf(stderr, "value %s is not an integer\n", *argv);
					return BCME_USAGE_ERROR;
				}
				if (words > SPROM_WORDS) {
					fprintf(stderr, "max of %d words\n", SPROM_WORDS);
					return BCME_USAGE_ERROR;
				}
			}

			bytes = 2 * words;
			memcpy(countptr, &bytes, sizeof(int));
		}
	}

	if (argc) {
		ret = dhd_set(dhd, DHD_SET_VAR, buf,
		              (strlen("sprom") + 1) + (2 * sizeof(int)) + bytes);
		return (ret);
	} else {
		ret = dhd_get(dhd, DHD_GET_VAR, buf,
		              (strlen("sprom") + 1) + (2 * sizeof(int)) + bytes);
		if (ret < 0) {
			return ret;
		}

		for (i = 0; i < (int)words; i++) {
			if ((i % 8) == 0)
				printf("\n  srom[%03d]:  ", i);
			printf("0x%04x  ", ((uint16*)buf)[i]);
		}
		printf("\n");
	}

	return 0;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

/*
 * read_vars: reads an environment variables file into a buffer,
 * reformatting them and returning the length (-1 on error).
 *
 * The input text file consists of lines of the form "<var>=<value>\n".
 * CRs are ignored, as are blank lines and comments beginning with '#'.
 *
 * The output buffer consists of blocks of the form "<var>=<value>\0"
 * (the newlines have been replaced by NULs)
 *
 * Todo: allow quoted variable names and quoted values.
*/

#if defined(BWL_FILESYSTEM_SUPPORT)
static int
read_vars(char *fname, char *buf, int buf_maxlen)
{
	FILE *fp;
	int buf_len, slen;
	char line[256], *s, *e;
	int line_no = 0;

	if ((fp = fopen(fname, "rb")) == NULL) {
		fprintf(stderr, "Cannot open NVRAM file %s: %s\n",
		        fname, strerror(errno));
		exit(1);
	}

	buf_len = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		bool found_eq = FALSE;

		/* Ensure line length is limited */
		line[sizeof(line) - 1] = 0;

		/* Skip any initial white space */
		for (s = line; *s == ' ' || *s == '\t'; s++)
			;

		/* Determine end of string */
		for (e = s; *e != 0 && *e != '#' && *e != '\r' && *e != '\n'; e++)
			if (*e == '=')
				found_eq = TRUE;

		/* Strip any white space from end of string */
		while (e > s && (e[-1] == ' ' || e[-1] == '\t'))
			e--;

		slen = e - s;

		/* Skip lines that end up blank */
		if (slen == 0)
			continue;

		if (!found_eq) {
			fprintf(stderr, "Invalid line %d in NVRAM file %s\n", line_no, fname);
			fclose(fp);
			return -1;
		}

		if (buf_len + slen + 1 > buf_maxlen) {
			fprintf(stderr, "NVRAM file %s too long\n", fname);
			fclose(fp);
			return -1;
		}

		memcpy(buf + buf_len, s, slen);
		buf_len += slen;
		buf[buf_len++] = 0;
	}

	fclose(fp);

	return buf_len;
}
#endif   /* BWL_FILESYSTEM_SUPPORT */

static int
dhd_vars(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	uint argc;
	char *bufp;

	UNUSED_PARAMETER(cmd);

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	switch (argc) {
	case 0: /* get */
	{
		if ((ret = dhd_var_getbuf(dhd, "vars", NULL, 0, (void**)&bufp)))
			break;
		while (*bufp) {
			printf("%s\n", bufp);
			bufp += strlen(bufp) + 1;
		}
	}
	break;

#if defined(BWL_FILESYSTEM_SUPPORT)
	case 1: /* set */
	{
		char *vname;
		uint nvram_len;

		vname = argv[1];

		bufp = buf;
		strcpy(bufp, "vars");
		bufp += strlen("vars") + 1;

		if ((ret = read_vars(vname, bufp,
		                           DHD_IOCTL_MAXLEN - (strlen("vars") + 3))) < 0) {
			ret = -1;
			break;
		}

		nvram_len = ret;
		bufp += nvram_len;
		*bufp++ = 0;

		ret = dhd_set(dhd, DHD_SET_VAR, buf, bufp - buf);
	}
	break;
#endif   /* BWL_FILESYSTEM_SUPPORT */

	default:
		ret = -1;
		break;
	}

	return ret;
}

#define MEMBLOCK 2048

/* Check that strlen("membytes")+1 + 2*sizeof(int32) + MEMBLOCK <= DHD_IOCTL_MAXLEN */
#if (MEMBLOCK + 17 > DHD_IOCTL_MAXLEN)
#error MEMBLOCK/DHD_IOCTL_MAXLEN sizing
#endif


#if defined(BWL_FILESYSTEM_SUPPORT)
static int
dhd_verify_file_bytes(void *dhd, uint8 *memblock, int start, uint len)
{
	int ret = 0;
	uint i = 0;
	char *ptr;
	int params[2];
	uint8 *src, *dst;

	params[0] = start;
	params[1] = len;
	ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
	if (ret) {
		fprintf(stderr, "%s: failed reading %d membytes from 0x%08x\n",
		__FUNCTION__, len, start);
		return ret;
	}

	src = (uint8 *)memblock;
	dst = (uint8 *)ptr;
	while (i < len) {
		if (src[i] != dst[i]) {
			fprintf(stderr, "   0x%x: exp[0x%02X] != got[0x%02X]\n",
				start+i, src[i], dst[i]);
			ret = -1;
		}
		i++;
	}

	return ret;
}

static int
dhd_load_file_bytes(void *dhd, cmd_t *cmd, FILE *fp, int fsize, int start, uint blk_sz, bool verify)
{
	int tot_len = 0;
	uint read_len;
	char *bufp;
	uint len;
	uint8 memblock[MEMBLOCK];
	int ret;
	int retry;

	UNUSED_PARAMETER(cmd);

	if (!fsize || !fp)
		return -1;

	assert(blk_sz <= MEMBLOCK);

	while (tot_len < fsize) {
		read_len = fsize - tot_len;
		if (read_len >= blk_sz) {
			read_len = blk_sz;

			if (!ISALIGNED(start, MEMBLOCK))
				read_len = ROUNDUP(start, MEMBLOCK) - start;
		}

		len = fread(memblock, sizeof(uint8), read_len, fp);
		if ((len < read_len) && !feof(fp)) {
			fprintf(stderr, "%s: error reading file\n", __FUNCTION__);
			return -1;

		}
		retry = 0;
failed_retry:

		bufp = buf;
		memset(bufp, 0, DHD_IOCTL_MAXLEN);
		strcpy(bufp, "membytes");
		bufp += strlen("membytes") + 1;
		memcpy(bufp, &start, sizeof(int));
		bufp += sizeof(int);
		memcpy(bufp, &len, sizeof(int));
		bufp += sizeof(int);
		memcpy(bufp, memblock, len);

		ret = dhd_set(dhd, DHD_SET_VAR, &buf[0], (bufp - buf + len));

		if (ret) {
			fprintf(stderr, "%s: error %d on writing %d membytes at 0x%08x\n",
			        __FUNCTION__, ret, len, start);
			return ret;
		}

		if (verify == TRUE) {
			if (dhd_verify_file_bytes(dhd, memblock, start, len) != 0) {
				if (retry++ < 5000)
				{
					fprintf(stderr, "%s: verify failed %d membytes "
						"from 0x%08x\n", __FUNCTION__, len, start);
					goto failed_retry;
				}
			}
		}

		start += len;
		tot_len += len;
	}
	return 0;
}
#endif   /* BWL_FILESYSTEM_SUPPORT */

#ifdef PROP_TXSTATUS
static int
dhd_proptxstatusenable(void *dhd, cmd_t *cmd, char **argv)
{
	int flag = 0xdead;
	int ret;

	if (argv[1]) {
		flag = atoi(argv[1]);
		ret = dhd_iovar_setint(dhd, cmd->name, flag);
	}
	else {
		ret = dhd_iovar_getint(dhd, cmd->name, &flag);
		if (ret >= 0)
			printf("proptxstatus: %d\n", flag);
	}
	return ret;
}

static int
dhd_proptxstatusmode(void *dhd, cmd_t *cmd, char **argv)
{
	int mode = 0xdead;
	int ret;

	if (argv[1]) {
		mode = atoi(argv[1]);
		ret = dhd_iovar_setint(dhd, cmd->name, mode);
	}
	else {
		ret = dhd_iovar_getint(dhd, cmd->name, &mode);
		if (ret >= 0)
			printf("proptxstatusmode: %d\n", mode);
	}
	return ret;
}

static int
dhd_proptxopt(void *dhd, cmd_t *cmd, char **argv)
{
	int flag = 0xdead;
	int ret;

	if (argv[1]) {
		flag = atoi(argv[1]);
		ret = dhd_iovar_setint(dhd, cmd->name, flag);
	}
	else {
		ret = dhd_iovar_getint(dhd, cmd->name, &flag);
		if (ret >= 0)
			printf("proptx_opt: %d\n", flag);
	}
	return ret;
}

#endif /* PROP_TXSTATUS */

static int
dhd_get_ramstart(void *dhd, uint32 *ramstart)
{
	int ret;
	char *ramstart_args[] = {"ramstart", NULL};

	/* Read the bus type the DHD driver is associated to */
	if ((ret = dhd_var_get(dhd, NULL, ramstart_args)) != BCME_OK) {
		fprintf(stderr, "%s: error obtaining ramstart\n", __FUNCTION__);

		return ret;
	}

	*ramstart = *(uint32 *)buf;

	return BCME_OK;
}

static int
dhd_download(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	bool reset = TRUE;
	bool run = TRUE;
	bool verify = FALSE;
	char *fname = NULL;
	char *vname = NULL;
	uint32 start;
	int ret = 0;
	int fsize;
	uint32 bustype;
	long filepos;

	FILE *fp = NULL;
	uint32 ramsize;
	char *memszargs[] = { "ramsize", NULL };

	char *bufp;

	miniopt_t opts;
	int opt_err;
	uint nvram_len;
	struct trx_header trx_hdr;
	uint32 trx_hdr_len;
	bool trx_file = FALSE;
	uint memblock_sz = MEMBLOCK;
	bool embedded_ucode = FALSE;

	UNUSED_PARAMETER(cmd);

	if ((ret = dhd_get_ramstart(dhd, &start)) != BCME_OK)
		goto exit;

	/* Parse command-line options */
	miniopt_init(&opts, "download", "", TRUE);

	argv++;
	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "download options error\n");
			ret = -1;
			goto exit;
		}
		argv += opts.consumed;

		if (opts.opt == 'a') {
			if (!opts.good_int) {
				fprintf(stderr, "invalid address %s\n", opts.valstr);
				ret = -1;
				goto exit;
			}
			start = (uint32)opts.uval;
		} else if (opts.positional) {
			if (fname && vname) {
				fprintf(stderr, "extra positional arg, %s\n",
				        opts.valstr);
				ret = -1;
				goto exit;
			}
			if (fname)
				vname = opts.valstr;
			else
				fname = opts.valstr;
		} else if (!opts.opt) {
			if (!strcmp(opts.key, "noreset")) {
				reset = FALSE;
			} else if (!strcmp(opts.key, "norun")) {
				run = FALSE;
			} else if (!strcmp(opts.key, "verify")) {
				verify = TRUE;
			} else {
				fprintf(stderr, "unrecognized option %s\n", opts.valstr);
				ret = -1;
				goto exit;
			}
		} else {
			fprintf(stderr, "unrecognized option %c\n", opts.opt);
			ret = -1;
			goto exit;
		}
	}

	/* validate arguments */
	if (!fname) {
		fprintf(stderr, "filename required\n");
		ret = -1;
		goto exit;
	}

	/* validate file size compared to memory size */
	if ((fsize = file_size(fname)) < 0) {
		ret = -1;
		goto exit;
	}
	/* read the file and push blocks down to memory */
	if ((fp = fopen(fname, "rb")) == NULL) {
		fprintf(stderr, "%s: unable to open %s: %s\n",
		        __FUNCTION__, fname, strerror(errno));
		ret = -1;
		goto exit;
	}
	/* Verify the file is a regular bin file or trx file */
	{
		uint32 tmp_len;
		trx_hdr_len = sizeof(struct trx_header);
		tmp_len = fread(&trx_hdr, sizeof(uint8), trx_hdr_len, fp);
		if (tmp_len == trx_hdr_len) {
			if (trx_hdr.magic == TRX_MAGIC) {
				trx_file = TRUE;
				if (trx_hdr.flag_version & TRX_EMBED_UCODE)
					embedded_ucode = TRUE;
			}
			else
				fseek(fp, 0, SEEK_SET);
		}
		else
			fseek(fp, 0, SEEK_SET);
	}

	/* Check on which bus the dhd driver is sitting. Downloading methodology differs from
	 * USB to SDIO.
	 */
	{
		char* bustype_args[] = {"bustype", NULL};

		/* Read the bus type the DHD driver is associated to */
		if ((ret = dhd_var_get(dhd, NULL, bustype_args))) {
			fprintf(stderr, "%s: error obtaining bustype\n", __FUNCTION__);
			goto exit;
		}

		bustype = *(uint32*)buf;
	}

	if (trx_file)
		fsize = (int)(trx_hdr.offsets[0]);

	if (bustype == BUS_TYPE_SDIO) {
		if ((ret = dhd_var_get(dhd, NULL, memszargs))) {
			fprintf(stderr, "%s: error obtaining ramsize\n", __FUNCTION__);
			goto exit;
		}
		ramsize = *(uint32*)buf;
	}


	BCM_REFERENCE(ramsize);

	/* do the download reset if not suppressed */
	if (reset) {
		if ((ret = dhd_iovar_setint(dhd, "dwnldstate", TRUE))) {
			fprintf(stderr, "%s: failed to put dongle in download mode\n",
			        __FUNCTION__);
			goto exit;
		}
	}
	
#define RDL_CHUNK	1500  /* size of each dl transfer */

	if (BUS_TYPE_USB == bustype) {
		/* store the cur pos pointing to base image which should be written */
		filepos = ftell(fp);
		if (filepos == -1) {
			fprintf(stderr, "%s: ftell failed.\n", __FUNCTION__);
		}

		/* In case of USB, we need to write header information also to dongle. */
		fseek(fp, 0, SEEK_SET);

		/* The file size is "base_image + TRX_Header_size" */
		fsize = (int)(trx_hdr.offsets[0] + sizeof(struct trx_header));

		memblock_sz = RDL_CHUNK;
	}


	/* Load the ram image */
	if ((ret = dhd_load_file_bytes(dhd, cmd, fp, fsize, start, memblock_sz, verify))) {
		fprintf(stderr, "%s: error loading the ramimage at addr 0x%x\n",
		        __FUNCTION__, start);
		goto exit;
	}

	if (trx_file) {

		filepos = ftell(fp);
		if (filepos == -1) {
			fprintf(stderr, "%s: ftell failed.\n", __FUNCTION__);
		}

		if (BUS_TYPE_SDIO == bustype) {

		}
	}

	fclose(fp);
	fp = NULL;

	/* download the vars file if specified */
	if (vname) {
		bufp = buf;
		strcpy(bufp, "vars");
		bufp += strlen("vars") + 1;

		if ((ret = read_vars(vname, bufp,
		                           DHD_IOCTL_MAXLEN - (strlen("vars") + 3))) < 0) {
			ret = -1;
			goto exit;
		}

		nvram_len = ret;
		bufp += nvram_len;
		*bufp++ = 0;

		ret = dhd_set(dhd, DHD_SET_VAR, buf, (bufp - buf));
		if (ret) {
			fprintf(stderr, "%s: error %d on delivering vars\n",
			        __FUNCTION__, ret);
			goto exit;
		}
	}

	/* start running the downloaded code if not suppressed */
	if (run) {
		if ((ret = dhd_iovar_setint(dhd, "dwnldstate", FALSE))) {

			fprintf(stderr, "%s: failed to take dongle out of download mode\n",
			        __FUNCTION__);
			/* USB Error return values */
			if (BUS_TYPE_USB == bustype) {
				if (ret == -1)
					fprintf(stderr, "%s: CPU is not in RUNNABLE State\n",
						__FUNCTION__);
				else
					fprintf(stderr, "%s: Error in setting CPU to RUN mode.\n",
						__FUNCTION__);
			}
			goto exit;
		}
	}
	if (embedded_ucode) {
	}

exit:
	if (fp)
		fclose(fp);

	return ret;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

static int
dhd_dldn(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	char *fname = NULL;
	uint32 start;
	int ret = 0;
	int fsize;
	int fd = 0;

	FILE *fp = NULL;
	uint32 ramsize;

	uint len;
	uint8 memblock[MEMBLOCK];

	miniopt_t opts;
	int opt_err;

	UNUSED_PARAMETER(cmd);

	/* Parse command-line options */
	miniopt_init(&opts, "download", "", TRUE);
	argv++;

	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "download options error\n");
			ret = -1;
			goto exit;
		}
		argv += opts.consumed;

		if (opts.positional) {
			if (fname) {
				fprintf(stderr, "extra positional arg, %s\n",
				        opts.valstr);
				ret = -1;
				goto exit;
			}
			if (!fname)
				fname = opts.valstr;
		} else {
			fprintf(stderr, "unrecognized option %c\n", opts.opt);
			ret = -1;
			goto exit;
		}
	}

	fd = dhd_set(dhd, DHD_DLDN_ST, NULL, 0);
	if (fd < 0) {
		ret = -1;
		goto exit;
	}

	/* validate arguments */
	if (!fname) {
		fprintf(stderr, "filename required\n");
		ret = -1;
		goto exit;
	}

	/* validate file size compared to memory size */
	if ((fsize = file_size(fname)) < 0) {
		ret = -1;
		goto exit;
	}

	ramsize = 393216;

	if (ramsize && ((uint32)fsize > ramsize)) {
		fprintf(stderr, "%s: file %s too large (%d > %d)\n",
		        __FUNCTION__, fname, fsize, ramsize);
		ret = -1;
		goto exit;
	}

	/* read the file and push blocks down to memory */
	if ((fp = fopen(fname, "rb")) == NULL) {
		fprintf(stderr, "%s: unable to open %s: %s\n",
		        __FUNCTION__, fname, strerror(errno));
		ret = -1;
		goto exit;
	}

	if ((ret = dhd_get_ramstart(dhd, &start)) != BCME_OK)
		goto exit;

	while ((len = fread(memblock, sizeof(uint8), MEMBLOCK, fp))) {
		if (len < MEMBLOCK && !feof(fp)) {
			fprintf(stderr, "%s: error reading file %s\n", __FUNCTION__, fname);
			ret = -1;
			goto exit;
		}

		ret = dhd_set(dhd, DHD_DLDN_WRITE, memblock, len);
		if (ret) {
			fprintf(stderr, "%s: error %d on writing %d membytes at 0x%08x\n",
			        __FUNCTION__, ret, len, start);
			goto exit;
		}

		start += len;
	}

	if (!feof(fp)) {
		fprintf(stderr, "%s: error reading file %s\n", __FUNCTION__, fname);
		ret = -1;
		goto exit;
	}
	fclose(fp);
	fp = NULL;

exit:
	if (fp)
		fclose(fp);

	if (fd)
		ret = dhd_set(dhd, DHD_DLDN_END, NULL, 0);

	return ret;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

static int
dhd_upload(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	char *fname = NULL;
	uint32 start;
	uint32 size = 0;
	int ret = 0;

	FILE *fp;
	uint32 ramsize;
	char *memszargs[] = { "ramsize", NULL };

	uint len;

	miniopt_t opts;
	int opt_err;

	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);

	if ((ret = dhd_get_ramstart(dhd, &start)) != BCME_OK)
		goto exit;

	/* Parse command-line options */
	miniopt_init(&opts, "upload", "", TRUE);

	argv++;
	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "upload options error\n");
			ret = -1;
			goto exit;
		}
		argv += opts.consumed;

		if (opts.opt == 'a') {
			if (!opts.good_int) {
				fprintf(stderr, "invalid address %s\n", opts.valstr);
				ret = -1;
				goto exit;
			}
			start = (uint32)opts.uval;
		} else if (opts.positional) {
			if (!fname) {
				fname = opts.valstr;
			} else if (opts.good_int) {
				size = (uint32)opts.uval;
			} else {
				fprintf(stderr, "upload options error\n");
				ret = -1;
				goto exit;
			}
		} else if (!opts.opt) {
			fprintf(stderr, "unrecognized option %s\n", opts.valstr);
			ret = -1;
			goto exit;
		} else {
			fprintf(stderr, "unrecognized option %c\n", opts.opt);
			ret = -1;
			goto exit;
		}
	}

	/* validate arguments */
	if (!fname) {
		fprintf(stderr, "filename required\n");
		ret = -1;
		goto exit;
	}

	if ((ret = dhd_var_get(dhd, NULL, memszargs))) {
		fprintf(stderr, "%s: error obtaining ramsize\n", __FUNCTION__);
		goto exit;
	}
	ramsize = *(uint32*)buf;

	if (!ramsize)
		ramsize = size;

	if ((fp = fopen(fname, "wb")) == NULL) {
		fprintf(stderr, "%s: Could not open %s: %s\n",
		        __FUNCTION__, fname, strerror(errno));
		ret = -1;
		goto exit;
	}

	/* default size to full RAM */
	if (!size)
		size = ramsize;

	/* read memory and write to file */
	while (size) {
		char *ptr;
		int params[2];

		len = MIN(MEMBLOCK, size);

		params[0] = start;
		params[1] = len;
		ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
		if (ret) {
			fprintf(stderr, "%s: failed reading %d membytes from 0x%08x\n",
			        __FUNCTION__, len, start);
			break;
		}

		if (fwrite(ptr, sizeof(char), len, fp) != len) {
			fprintf(stderr, "%s: error writing to file %s\n", __FUNCTION__, fname);
			ret = -1;
			break;
		}

		start += len;
		size -= len;
	}

	fclose(fp);
exit:
	return ret;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

#ifdef BWL_FILESYSTEM_SUPPORT
static int
dhd_get_debug_info(void *dhd, hndrte_debug_t *debug_info)
{
	int i;
	int ret;
	int params[2];

	uint32 *buffer;
	uint32 debug_info_ptr;
	uint32 ramstart;

	if ((ret = dhd_get_ramstart(dhd, &ramstart)) != BCME_OK)
		return ret;

	/*
	 * Different chips have different fixed debug_info_ptrs
	 * because of different ROM locations/uses.  Try them all looking
	 * for the magic number.
	 */
	for (i = 0; ; i++) {
		if (debug_info_ptrs[i] == DEBUG_INFO_PTRS_END) {
			fprintf(stderr, "Error: cannot find pointer to debug_info\n");
			return -1;
		}

		params[0] = debug_info_ptrs[i] + ramstart;
		params[1] = 8;
		ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&buffer);
		if ((ret == 0) &&
		    (*buffer == HNDRTE_DEBUG_PTR_PTR_MAGIC)) {
			break;
		}
	}

	debug_info_ptr = *(buffer + 1);
	if (debug_info_ptr == 0) {
		fprintf(stderr, "Error: Debug info pointer is zero\n");
		return -1;
	}

	/* Read the area the debuginfoptr points at */
	params[0] = debug_info_ptr;
	params[1] = sizeof(hndrte_debug_t);
	ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&buffer);
	if (ret) {
		fprintf(stderr, "%s: failed reading %lu membytes from 0x%08lx\n",
			__FUNCTION__, (long unsigned) params[1], (long unsigned) params[0]);
		return ret;
	}

	memcpy((char *) debug_info, buffer, sizeof(hndrte_debug_t));

	/* Sanity check the area */
	if ((debug_info->magic != HNDRTE_DEBUG_MAGIC) ||
	    (debug_info->version != HNDRTE_DEBUG_VERSION)) {
		fprintf(stderr, "Error: Invalid debug info area\n");
		return -1;
	}

	return 0;
}
#endif /* BWL_FILESYSTEM_SUPPORT */

static int
dhd_coredump(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	char *fname = NULL;
	int ret;

	FILE *fp;

	hndrte_debug_t debugInfo;

	miniopt_t opts;
	int opt_err;

	int params[2];
	char *ptr;

	unsigned int start;
	unsigned int size;

	prstatus_t prstatus;

	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);

	/* Parse command-line options */
	miniopt_init(&opts, "dump", "", TRUE);

	argv++;
	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "dump options error\n");
			ret = -1;
			goto exit;
		}
		argv += opts.consumed;

		if (opts.positional) {
			if (!fname) {
				fname = opts.valstr;
			} else {
				fprintf(stderr, "dump options error\n");
				ret = -1;
				goto exit;
			}
		} else if (!opts.opt) {
			fprintf(stderr, "unrecognized option %s\n", opts.valstr);
			ret = -1;
			goto exit;
		} else {
			fprintf(stderr, "unrecognized option %c\n", opts.opt);
			ret = -1;
			goto exit;
		}
	}

	/* validate arguments */
	if (!fname) {
		fprintf(stderr, "filename required\n");
		ret = -1;
		goto exit;
	}

	if ((ret = dhd_get_debug_info(dhd, &debugInfo)) < 0)
		goto exit;

	/* Get the base and size to dump */
	start = debugInfo.ram_base;
	size = debugInfo.ram_size;

	/* Get the arm trap area */
	bzero(&prstatus, sizeof(prstatus_t));
	if (debugInfo.trap_ptr != 0) {
		int i;
		trap_t armtrap;
		uint32 *reg;

		params[0] = debugInfo.trap_ptr;
		params[1] = sizeof(trap_t);
		ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
		if (ret) {
			fprintf(stderr, "%s: failed reading %lu membytes from 0x%08lx\n",
				__FUNCTION__, (long unsigned) params[1], (long unsigned) params[0]);
			goto exit;
		}

		memcpy((char *) &armtrap, ptr, sizeof(trap_t));

		/* Populate the prstatus */
		prstatus.si_signo = armtrap.type;
		reg = &armtrap.r0;
		for (i = 0; i < 15; i++, reg++) {
			prstatus.uregs[i] = *reg;
		}
		prstatus.uregs[15] = armtrap.epc;
	}

	if ((fp = fopen(fname, "wb")) == NULL) {
		fprintf(stderr, "%s: Could not open %s: %s\n",
		        __FUNCTION__, fname, strerror(errno));
		ret = -1;
		goto exit;
	}

	/* Write the preamble and debug header */
	fprintf(fp, "Dump starts for version %s FWID 01-%x\n", debugInfo.epivers, debugInfo.fwid);
	fprintf(fp, "XXXXXXXXXXXXXXXXXXXX");
	fprintf(fp, "%8.8lX", (long unsigned) sizeof(debugInfo));
	if (fwrite(&debugInfo, sizeof(unsigned char), sizeof(debugInfo), fp) != sizeof(debugInfo)) {
		fprintf(stderr, "%s: error writing to file %s\n", __FUNCTION__, fname);
		ret = -1;
		fclose(fp);
		goto exit;
	}

	/* Write the prstatus */
	if (fwrite(&prstatus, sizeof(unsigned char), sizeof(prstatus), fp) != sizeof(prstatus)) {
		fprintf(stderr, "%s: error writing to file %s\n", __FUNCTION__, fname);
		ret = -1;
		fclose(fp);
		goto exit;
	}

	/* Write the ram size as another sanity check */
	fprintf(fp, "%8.8X", size);

	/* read memory and write to file */
	while (size) {
		int len;
		len = MIN(MEMBLOCK, size);

		params[0] = start;
		params[1] = len;
		ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
		if (ret) {
			fprintf(stderr, "%s: failed reading %d membytes from 0x%08x\n",
			        __FUNCTION__, len, start);
			break;
		}

		if (fwrite(ptr, sizeof(char), len, fp) != (uint) len) {
			fprintf(stderr, "%s: error writing to file %s\n", __FUNCTION__, fname);
			ret = -1;
			break;
		}

		start += len;
		size -= len;
	}

	fclose(fp);
exit:
	return ret;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

static int
dhd_consoledump(void *dhd, cmd_t *cmd, char **argv)
{
#if !defined(BWL_FILESYSTEM_SUPPORT)
	return (-1);
#else
	int ret;

	hndrte_debug_t debugInfo;

	miniopt_t opts;
	int opt_err;

	int params[2];
	char *ptr;

	unsigned int start;
	unsigned int size;
	int len;

	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);

	/* Parse command-line options */
	miniopt_init(&opts, "consoledump", "", TRUE);

	argv++;
	while ((opt_err = miniopt(&opts, argv)) != -1) {
		if (opt_err == 1) {
			fprintf(stderr, "dump options error\n");
			ret = -1;
			goto exit;
		}
		argv += opts.consumed;

		if (!opts.opt) {
			fprintf(stderr, "unrecognized option %s\n", opts.valstr);
			ret = -1;
			goto exit;
		} else {
			fprintf(stderr, "unrecognized option %c\n", opts.opt);
			ret = -1;
			goto exit;
		}
	}

	if ((ret = dhd_get_debug_info(dhd, &debugInfo)) < 0)
		goto exit;

	if (debugInfo.console <= debugInfo.ram_base) {
		fprintf(stderr, "%s: console not found\n", __FUNCTION__);
		ret = -1;
		goto exit;
	}

	/* Get the debug console area */
	params[0] = debugInfo.console;
	params[1] = sizeof(hndrte_cons_t);
	ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
	if (ret) {
		fprintf(stderr, "%s: failed reading %lu membytes from 0x%08lx\n",
			__FUNCTION__, (long unsigned) params[1], (long unsigned) params[0]);
		goto exit;
	}

	if (ptr == NULL) {
		fprintf(stderr, "%s: console not initialised\n", __FUNCTION__);
		ret = -1;
		goto exit;
	}

	start = (unsigned int)((hndrte_cons_t *)ptr)->log.buf;
	size = ((hndrte_cons_t *)ptr)->log.buf_size;

	if (start <= debugInfo.ram_base) {
		fprintf(stderr, "%s: console buffer not initialised\n", __FUNCTION__);
		ret = -1;
		goto exit;
	}

	/* read memory and write to file */
	while (size > 0) {
		len = MIN(MEMBLOCK, size);

		params[0] = start;
		params[1] = len;
		ret = dhd_var_getbuf(dhd, "membytes", params, 2 * sizeof(int), (void**)&ptr);
		if (ret) {
			fprintf(stderr, "%s: failed reading %d membytes from 0x%08x\n",
			        __FUNCTION__, len, start);
			break;
		}

		printf("%s", ptr);

		start += len;
		size -= len;
	}

exit:
	return ret;
#endif /* BWL_FILESYSTEM_SUPPORT */
}

static int
dhd_logstamp(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	char *endptr = NULL;
	uint argc;
	int valn[2] = {0, 0};

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--; argv++;

	if (argc > 2)
		return BCME_USAGE_ERROR;

	if (argc) {
		valn[0] = strtol(argv[0], &endptr, 0);
		if (*endptr != '\0') {
			printf("bad val1: %s\n", argv[0]);
			return BCME_USAGE_ERROR;
		}
	}

	if (argc > 1) {
		valn[1] = strtol(argv[1], &endptr, 0);
		if (*endptr != '\0') {
			printf("bad val2: %s\n", argv[1]);
			return BCME_USAGE_ERROR;
		}
	}

	ret = dhd_var_setbuf(dhd, cmd->name, valn, argc * sizeof(int));

	return (ret);
}

static int
dhd_sd_reg(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	sdreg_t sdreg;
	char *endptr = NULL;
	uint argc;
	void *ptr = NULL;

	bzero(&sdreg, sizeof(sdreg));

	/* arg count */
	for (argc = 0; argv[argc]; argc++);
	argc--;

	/* hostreg: offset [value]; devreg: func offset [value] */
	if (!strcmp(cmd->name, "sd_hostreg")) {
		argv++;
		if (argc < 1) {
			printf("required args: offset [value]\n");
			return BCME_USAGE_ERROR;
		}

	} else if (!strcmp(cmd->name, "sd_devreg")) {
		argv++;
		if (argc < 2) {
			printf("required args: func offset [value]\n");
			return BCME_USAGE_ERROR;
		}

		sdreg.func = strtoul(*argv++, &endptr, 0);
		if (*endptr != '\0') {
			printf("Invalid function number\n");
			return BCME_USAGE_ERROR;
		}
	} else {
		return BCME_USAGE_ERROR;
	}

	sdreg.offset = strtoul(*argv++, &endptr, 0);
	if (*endptr != '\0') {
		printf("Invalid offset value\n");
		return BCME_USAGE_ERROR;
	}

	/* third arg: value */
	if (*argv) {
		sdreg.value = strtoul(*argv, &endptr, 0);
		if (*endptr != '\0') {
			printf("Invalid value\n");
			return BCME_USAGE_ERROR;
		}
	}

	/* no third arg means get, otherwise set */
	if (!*argv) {
		if ((ret = dhd_var_getbuf(dhd, cmd->name, &sdreg, sizeof(sdreg), &ptr)) >= 0)
			printf("0x%x\n", *(int *)ptr);
	} else {
		ret = dhd_var_setbuf(dhd, cmd->name, &sdreg, sizeof(sdreg));
	}

	return (ret);
}

static dbg_msg_t dhd_msgs[] = {
	{DHD_ERROR_VAL,	"error"},
	{DHD_ERROR_VAL, "err"},
	{DHD_TRACE_VAL, "trace"},
	{DHD_INFO_VAL,	"inform"},
	{DHD_INFO_VAL,	"info"},
	{DHD_INFO_VAL,	"inf"},
	{DHD_DATA_VAL,	"data"},
	{DHD_CTL_VAL,	"ctl"},
	{DHD_TIMER_VAL,	"timer"},
	{DHD_HDRS_VAL,	"hdrs"},
	{DHD_BYTES_VAL,	"bytes"},
	{DHD_INTR_VAL,	"intr"},
	{DHD_LOG_VAL,	"log"},
	{DHD_GLOM_VAL,	"glom"},
	{DHD_EVENT_VAL,	"event"},
	{DHD_BTA_VAL,	"bta"},
	{DHD_ISCAN_VAL,	"iscan"},
	{DHD_ARPOE_VAL,	"arpoe"},
	{DHD_REORDER_VAL, "reorder"},
	{0,		NULL}
};

static int
dhd_msglevel(void *dhd, cmd_t *cmd, char **argv)
{
	return dhd_do_msglevel(dhd, cmd, argv, dhd_msgs);
}

static int
dhd_do_msglevel(void *dhd, cmd_t *cmd, char **argv, dbg_msg_t *dbg_msg)
{
	int ret, i;
	uint val, last_val = 0, msglevel = 0, msglevel_add = 0, msglevel_del = 0;
	char *endptr = NULL;

	if ((ret = dhd_iovar_getint(dhd, cmd->name, (int*)&msglevel)) < 0)
		return (ret);

	if (!*++argv) {
		printf("0x%x ", msglevel);
		for (i = 0; (val = dbg_msg[i].value); i++) {
			if ((msglevel & val) && (val != last_val))
				printf(" %s", dbg_msg[i].string);
			last_val = val;
		}
		printf("\n");
		return (0);
	}

	while (*argv) {
		char *s = *argv;
		if (*s == '+' || *s == '-')
			s++;
		else
			msglevel_del = ~0;	/* make the whole list absolute */
		val = strtoul(s, &endptr, 0);
		/* not a plain integer if not all the string was parsed by strtoul */
		if (*endptr != '\0') {
			for (i = 0; (val = dbg_msg[i].value); i++)
				if (stricmp(dbg_msg[i].string, s) == 0)
					break;
			if (!val)
				goto usage;
		}
		if (**argv == '-')
			msglevel_del |= val;
		else
			msglevel_add |= val;
		++argv;
	}

	msglevel &= ~msglevel_del;
	msglevel |= msglevel_add;

	return (dhd_iovar_setint(dhd, cmd->name, msglevel));

usage:
	fprintf(stderr, "msg values may be a list of numbers or names from the following set.\n");
	fprintf(stderr, "Use a + or - prefix to make an incremental change.");

	for (i = 0; (val = dbg_msg[i].value); i++) {
		if (val != last_val)
			fprintf(stderr, "\n0x%04x %s", val, dbg_msg[i].string);
		else
			fprintf(stderr, ", %s", dbg_msg[i].string);
		last_val = val;
	}
	fprintf(stderr, "\n");

	return 0;
}

static char *
ver2str(unsigned int vms, unsigned int vls)
{
	static char verstr[100];
	unsigned int maj, year, month, day, build;

	maj = (vms >> 16) & 0xFFFF;
	if (maj > 1000) {
		/* it is probably a date... */
		year = (vms >> 16) & 0xFFFF;
		month = vms & 0xFFFF;
		day = (vls >> 16) & 0xFFFF;
		build = vls & 0xFFFF;
		sprintf(verstr, "%d/%d/%d build %d",
			month, day, year, build);
	} else {
		/* it is a tagged release. */
		sprintf(verstr, "%d.%d RC%d.%d",
			(vms>>16)&0xFFFF, vms&0xFFFF,
			(vls>>16)&0xFFFF, vls&0xFFFF);
	}
	return verstr;
}

static int
dhd_version(void *dhd, cmd_t *cmd, char **argv)
{
	int ret;
	char *ptr;

	UNUSED_PARAMETER(cmd);
	UNUSED_PARAMETER(argv);

	/* Display the application version info */
	printf("%s: %s\n", dhdu_av0,
		ver2str((EPI_MAJOR_VERSION << 16)| EPI_MINOR_VERSION,
		(EPI_RC_NUMBER << 16) | EPI_INCREMENTAL_NUMBER));

	if ((ret = dhd_var_getbuf(dhd, cmd->name, NULL, 0, (void**)&ptr)) < 0)
		return ret;

	/* Display the returned string */
	printf("%s\n", ptr);

	return 0;
}

static int
dhd_var_setint(void *dhd, cmd_t *cmd, char **argv)
{
	int32 val;
	int len;
	char *varname;
	char *endptr = NULL;
	char *p;

	if (cmd->set == -1) {
		printf("set not defined for %s\n", cmd->name);
		return BCME_ERROR;
	}

	if (!*argv) {
		printf("set: missing arguments\n");
		return BCME_USAGE_ERROR;
	}

	varname = *argv++;

	if (!*argv) {
		printf("set: missing value argument for set of \"%s\"\n", varname);
		return BCME_USAGE_ERROR;
	}

	val = strtol(*argv, &endptr, 0);
	if (*endptr != '\0') {
		/* not all the value string was parsed by strtol */
		printf("set: error parsing value \"%s\" as an integer for set of \"%s\"\n",
			*argv, varname);
		return BCME_USAGE_ERROR;
	}

	strcpy(buf, varname);
	p = buf;
	while (*p != '\0') {
		*p = tolower(*p);
		p++;
	}

	/* skip the NUL */
	p++;

	memcpy(p, &val, sizeof(uint));
	len = (p - buf) +  sizeof(uint);

	return (dhd_set(dhd, DHD_SET_VAR, &buf[0], len));
}

static int
dhd_var_get(void *dhd, cmd_t *cmd, char **argv)
{
	char *varname;
	char *p;

	UNUSED_PARAMETER(cmd);

	if (!*argv) {
		printf("get: missing arguments\n");
		return BCME_USAGE_ERROR;
	}

	varname = *argv++;

	if (*argv) {
		printf("get: error, extra arg \"%s\"\n", *argv);
		return BCME_USAGE_ERROR;
	}

	strcpy(buf, varname);
	p = buf;
	while (*p != '\0') {
		*p = tolower(*p);
		p++;
	}
	return (dhd_get(dhd, DHD_GET_VAR, &buf[0], DHD_IOCTL_MAXLEN));
}

static int
dhd_var_getint(void *dhd, cmd_t *cmd, char **argv)
{
	int err;
	int32 val;
	if (cmd->get == -1) {
		printf("get not defined for %s\n", cmd->name);
		return BCME_ERROR;
	}

	if ((err = dhd_var_get(dhd, cmd, argv)))
		return (err);

	val = *(int32*)buf;

	if (val < 10)
		printf("%d\n", val);
	else
		printf("%d (0x%x)\n", val, val);

	return (0);
}

static int
dhd_var_getandprintstr(void *dhd, cmd_t *cmd, char **argv)
{
	int err;

	if ((err = dhd_var_get(dhd, cmd, argv)))
		return (err);

	printf("%s\n", buf);
	return (0);
}


void
dhd_printlasterror(void *dhd)
{
	char *cmd[2] = {"bcmerrorstr"};

	if (dhd_var_get(dhd, NULL, cmd) != 0) {
		fprintf(stderr, "%s: \nError getting the last error\n", dhdu_av0);
	} else {
		fprintf(stderr, "%s: %s\n", dhdu_av0, buf);
	}
}

static int
dhd_varint(void *dhd, cmd_t *cmd, char *argv[])
{
	if (argv[1])
		return (dhd_var_setint(dhd, cmd, argv));
	else
		return (dhd_var_getint(dhd, cmd, argv));
}

static int
dhd_var_getbuf(void *dhd, char *iovar, void *param, int param_len, void **bufptr)
{
	int len;

	memset(buf, 0, DHD_IOCTL_MAXLEN);
	strcpy(buf, iovar);

	/* include the NUL */
	len = strlen(iovar) + 1;

	if (param_len)
		memcpy(&buf[len], param, param_len);

	*bufptr = buf;

	return dhd_get(dhd, DHD_GET_VAR, &buf[0], DHD_IOCTL_MAXLEN);
}

static int
dhd_var_setbuf(void *dhd, char *iovar, void *param, int param_len)
{
	int len;

	memset(buf, 0, DHD_IOCTL_MAXLEN);
	strcpy(buf, iovar);

	/* include the NUL */
	len = strlen(iovar) + 1;

	if (param_len)
		memcpy(&buf[len], param, param_len);

	len += param_len;

	return dhd_set(dhd, DHD_SET_VAR, &buf[0], len);
}

static int
dhd_var_void(void *dhd, cmd_t *cmd, char **argv)
{
	UNUSED_PARAMETER(argv);

	if (cmd->set < 0)
		return BCME_ERROR;

	return dhd_var_setbuf(dhd, cmd->name, NULL, 0);
}

/*
 * format an iovar buffer
 */
static uint
dhd_iovar_mkbuf(char *name, char *data, uint datalen, char *buf, uint buflen, int *perr)
{
	uint len;

	len = strlen(name) + 1;

	/* check for overflow */
	if ((len + datalen) > buflen) {
		*perr = BCME_BUFTOOSHORT;
		return 0;
	}

	strcpy(buf, name);

	/* append data onto the end of the name string */
	if (datalen > 0)
		memcpy(&buf[len], data, datalen);

	len += datalen;

	*perr = 0;
	return len;
}

static int
dhd_iovar_getint(void *dhd, char *name, int *var)
{
	char ibuf[DHD_IOCTL_SMLEN];
	int error;

	dhd_iovar_mkbuf(name, NULL, 0, ibuf, sizeof(ibuf), &error);
	if (error)
		return error;

	if ((error = dhd_get(dhd, DHD_GET_VAR, &ibuf, sizeof(ibuf))) < 0)
		return error;

	memcpy(var, ibuf, sizeof(int));

	return 0;
}

static int
dhd_iovar_setint(void *dhd, char *name, int var)
{
	int len;
	char ibuf[DHD_IOCTL_SMLEN];
	int error;

	len = dhd_iovar_mkbuf(name, (char *)&var, sizeof(var), ibuf, sizeof(ibuf), &error);
	if (error)
		return error;

	if ((error = dhd_set(dhd, DHD_SET_VAR, &ibuf, len)) < 0)
		return error;

	return 0;
}

static int
dhd_varstr(void *dhd, cmd_t *cmd, char **argv)
{
	int error;
	char *str;

	if (!*++argv) {
		void *ptr;

		if ((error = dhd_var_getbuf(dhd, cmd->name, NULL, 0, &ptr)) < 0)
			return (error);

		str = (char *)ptr;
		printf("%s\n", str);
		return (0);
	} else {
		str = *argv;
		/* iovar buffer length includes NUL */
		return dhd_var_setbuf(dhd, cmd->name, str, strlen(str) + 1);
	}
}


static int
dhd_hostreorder_flows(void *dhd, cmd_t *cmd, char **argv)
{
	int ret, count, i = 0;
	void *ptr;
	uint8 *flow_id;


	if ((ret = dhd_var_getbuf(dhd, cmd->name, NULL, 0, &ptr)) < 0) {
		printf("error getting reorder flows from the host\n");
		return ret;
	}
	flow_id = (uint8 *)ptr;
	count = *flow_id;
	if (!count)
		printf("there are no active flows\n");
	else {
		printf("flows(%d): \t", count);
		while (i++ < count)
			printf("%d  ", *flow_id++);
		printf("\n");
	}
	return 0;
}



/* These two utility functions are used by dhdu_linux.c
 * The code is taken from wlu.c.
 */
int
dhd_atoip(const char *a, struct ipv4_addr *n)
{
	char *c;
	int i = 0;

	for (;;) {
	        n->addr[i++] = (uint8)strtoul(a, &c, 0);
	        if (*c++ != '.' || i == IPV4_ADDR_LEN)
	                break;
	        a = c;
	}
	return (i == IPV4_ADDR_LEN);
}

int
dhd_ether_atoe(const char *a, struct ether_addr *n)
{
	char *c;
	int i = 0;

	memset(n, 0, ETHER_ADDR_LEN);
	for (;;) {
	        n->octet[i++] = (uint8)strtoul(a, &c, 16);
	        if (!*c++ || i == ETHER_ADDR_LEN)
	                break;
	        a = c;
	}
	return (i == ETHER_ADDR_LEN);
}
