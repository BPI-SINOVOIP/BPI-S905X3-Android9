#ifdef	STRACE_TESTS_H

# define TD	0
# define TF	0
# define TI	0
# define TN	0
# define TP	0
# define TS	0
# define TM	0
# define TST	0
# define TLST	0
# define TFST	0
# define TSTA	0
# define TSF	0
# define TFSF	0
# define TSFA	0
# define PU	0
# define NF	0
# define MA	0
# define SI	0
# define SE	0
# define CST	0
# define SEN(a)	0, 0

#else	/*	!STRACE_TESTS_H	*/

# define TD	TRACE_DESC
# define TF	TRACE_FILE
# define TI	TRACE_IPC
# define TN	TRACE_NETWORK
# define TP	TRACE_PROCESS
# define TS	TRACE_SIGNAL
# define TM	TRACE_MEMORY
# define TST	TRACE_STAT
# define TLST	TRACE_LSTAT
# define TFST	TRACE_FSTAT
# define TSTA	TRACE_STAT_LIKE
# define TSF	TRACE_STATFS
# define TFSF	TRACE_FSTATFS
# define TSFA	TRACE_STATFS_LIKE
# define PU	TRACE_PURE
# define NF	SYSCALL_NEVER_FAILS
# define MA	MAX_ARGS
# define SI	STACKTRACE_INVALIDATE_CACHE
# define SE	STACKTRACE_CAPTURE_ON_ENTER
# define CST	COMPAT_SYSCALL_TYPES
/* SEN(a) is defined elsewhere */

#endif
