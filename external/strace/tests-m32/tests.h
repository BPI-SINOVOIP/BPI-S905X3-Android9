/*
 * Copyright (c) 2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2016-2018 The strace developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STRACE_TESTS_H
#define STRACE_TESTS_H

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <sys/types.h>
# include "kernel_types.h"
# include "gcc_compat.h"
# include "macros.h"

/*
 * The printf-like function to use in header files
 * shared between strace and its tests.
 */
#ifndef STRACE_PRINTF
# define STRACE_PRINTF printf
#endif

/* Tests of "strace -v" are expected to define VERBOSE to 1. */
#ifndef VERBOSE
# define VERBOSE 0
#endif

#ifndef DEFAULT_STRLEN
/* Default maximum # of bytes printed in printstr et al. */
# define DEFAULT_STRLEN 32
#endif

/* Cached sysconf(_SC_PAGESIZE). */
size_t get_page_size(void);

/* The size of kernel's sigset_t. */
unsigned int get_sigset_size(void);

/* Print message and strerror(errno) to stderr, then exit(1). */
void perror_msg_and_fail(const char *, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2)) ATTRIBUTE_NORETURN;
/* Print message to stderr, then exit(1). */
void error_msg_and_fail(const char *, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2)) ATTRIBUTE_NORETURN;
/* Print message to stderr, then exit(77). */
void error_msg_and_skip(const char *, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2)) ATTRIBUTE_NORETURN;
/* Print message and strerror(errno) to stderr, then exit(77). */
void perror_msg_and_skip(const char *, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2)) ATTRIBUTE_NORETURN;

#ifndef perror_msg_and_fail
# define perror_msg_and_fail(fmt_, ...) \
	perror_msg_and_fail("%s:%d: " fmt_, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#ifndef perror_msg_and_fail
# define error_msg_and_fail(fmt_, ...) \
	error_msg_and_fail("%s:%d: " fmt_, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/* Stat the specified file and skip the test if the stat call failed. */
void skip_if_unavailable(const char *);

/*
 * Allocate memory that ends on the page boundary.
 * Pages allocated by this call are preceded by an unmapped page
 * and followed also by an unmapped page.
 */
void *tail_alloc(const size_t)
	ATTRIBUTE_MALLOC ATTRIBUTE_ALLOC_SIZE((1));
/* Allocate memory using tail_alloc, then memcpy. */
void *tail_memdup(const void *, const size_t)
	ATTRIBUTE_MALLOC ATTRIBUTE_ALLOC_SIZE((2));

/*
 * Allocate an object of the specified type at the end
 * of a mapped memory region.
 * Assign its address to the specified constant pointer.
 */
#define TAIL_ALLOC_OBJECT_CONST_PTR(type_name, type_ptr)	\
	type_name *const type_ptr = tail_alloc(sizeof(*type_ptr))

/*
 * Allocate an object of the specified type at the end
 * of a mapped memory region.
 * Assign its address to the specified variable pointer.
 */
#define TAIL_ALLOC_OBJECT_VAR_PTR(type_name, type_ptr)		\
	type_name *type_ptr = tail_alloc(sizeof(*type_ptr))

/*
 * Fill memory (pointed by ptr, having size bytes) with different bytes (with
 * values starting with start and resetting every period) in order to catch
 * sign, byte order and/or alignment errors.
 */
void fill_memory_ex(void *ptr, size_t size, unsigned char start,
		    unsigned char period);
/* Shortcut for fill_memory_ex(ptr, size, 0x80, 0x80) */
void fill_memory(void *ptr, size_t size);

/* Close stdin, move stdout to a non-standard descriptor, and print. */
void tprintf(const char *, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2));

/* Make a hexdump copy of C string */
const char *hexdump_strdup(const char *);

/* Make a hexdump copy of memory */
const char *hexdump_memdup(const char *, size_t);

/* Make a hexquoted copy of a string */
const char *hexquote_strndup(const char *, size_t);

/* Return inode number of socket descriptor. */
unsigned long inode_of_sockfd(int);

/* Print string in a quoted form. */
void print_quoted_string(const char *);

/*
 * Print a NUL-terminated string `str' of length up to `size' - 1
 * in a quoted form.
 */
void print_quoted_cstring(const char *str, size_t size);

/* Print memory in a quoted form. */
void print_quoted_memory(const void *, size_t);

/* Print memory in a hexquoted form. */
void print_quoted_hex(const void *, size_t);

/* Print time_t and nanoseconds in symbolic format. */
void print_time_t_nsec(time_t, unsigned long long, int);

/* Print time_t and microseconds in symbolic format. */
void print_time_t_usec(time_t, unsigned long long, int);

/* Read an int from the file. */
int read_int_from_file(const char *, int *);

/* Check whether given uid matches kernel overflowuid. */
void check_overflowuid(const int);

/* Check whether given gid matches kernel overflowgid. */
void check_overflowgid(const int);

/* Translate errno to its name. */
const char *errno2name(void);

/* Translate signal number to its name. */
const char *signal2name(int);

/* Print return code and, in case return code is -1, errno information. */
const char *sprintrc(long rc);
/* sprintrc variant suitable for usage as part of grep pattern. */
const char *sprintrc_grep(long rc);

struct xlat;

/* Print flags in symbolic form according to xlat table. */
int printflags(const struct xlat *, const unsigned long long, const char *);

/* Print constant in symbolic form according to xlat table. */
int printxval(const struct xlat *, const unsigned long long, const char *);

/* Invoke a socket syscall, either directly or via __NR_socketcall. */
int socketcall(const int nr, const int call,
	       long a1, long a2, long a3, long a4, long a5);

/* Wrappers for recvmmsg and sendmmsg syscalls. */
struct mmsghdr;
struct timespec;
int recv_mmsg(int, struct mmsghdr *, unsigned int, unsigned int, struct timespec *);
int send_mmsg(int, struct mmsghdr *, unsigned int, unsigned int);

/* Create a netlink socket. */
int create_nl_socket_ext(int proto, const char *name);
#define create_nl_socket(proto)	create_nl_socket_ext((proto), #proto)

/* Create a pipe with maximized descriptor numbers. */
void pipe_maxfd(int pipefd[2]);

/* if_nametoindex("lo") */
unsigned int ifindex_lo(void);

#ifdef HAVE_IF_INDEXTONAME
# define IFINDEX_LO_STR "if_nametoindex(\"lo\")"
#else
# define IFINDEX_LO_STR "1"
#endif

#define F8ILL_KULONG_SUPPORTED	(sizeof(void *) < sizeof(kernel_ulong_t))
#define F8ILL_KULONG_MASK	((kernel_ulong_t) 0xffffffff00000000ULL)

/*
 * For 64-bit kernel_ulong_t and 32-bit pointer,
 * return a kernel_ulong_t value by filling higher bits.
 * For other architertures, return the original pointer.
 */
static inline kernel_ulong_t
f8ill_ptr_to_kulong(const void *const ptr)
{
	const unsigned long uptr = (unsigned long) ptr;
	return F8ILL_KULONG_SUPPORTED
	       ? F8ILL_KULONG_MASK | uptr : (kernel_ulong_t) uptr;
}

# define LENGTH_OF(arg) ((unsigned int) sizeof(arg) - 1)

/* Zero-extend a signed integer type to unsigned long long. */
#define zero_extend_signed_to_ull(v) \
	(sizeof(v) == sizeof(char) ? (unsigned long long) (unsigned char) (v) : \
	 sizeof(v) == sizeof(short) ? (unsigned long long) (unsigned short) (v) : \
	 sizeof(v) == sizeof(int) ? (unsigned long long) (unsigned int) (v) : \
	 sizeof(v) == sizeof(long) ? (unsigned long long) (unsigned long) (v) : \
	 (unsigned long long) (v))

/* Sign-extend an unsigned integer type to long long. */
#define sign_extend_unsigned_to_ll(v) \
	(sizeof(v) == sizeof(char) ? (long long) (char) (v) : \
	 sizeof(v) == sizeof(short) ? (long long) (short) (v) : \
	 sizeof(v) == sizeof(int) ? (long long) (int) (v) : \
	 sizeof(v) == sizeof(long) ? (long long) (long) (v) : \
	 (long long) (v))

# define SKIP_MAIN_UNDEFINED(arg) \
	int main(void) { error_msg_and_skip("undefined: %s", arg); }

# if WORDS_BIGENDIAN
#  define LL_PAIR(HI, LO) (HI), (LO)
# else
#  define LL_PAIR(HI, LO) (LO), (HI)
# endif
# define LL_VAL_TO_PAIR(llval) LL_PAIR((long) ((llval) >> 32), (long) (llval))

# define _STR(_arg) #_arg
# define ARG_STR(_arg) (_arg), #_arg
# define ARG_ULL_STR(_arg) _arg##ULL, #_arg

/*
 * Assign an object of type DEST_TYPE at address DEST_ADDR
 * using memcpy to avoid potential unaligned access.
 */
#define SET_STRUCT(DEST_TYPE, DEST_ADDR, ...)						\
	do {										\
		DEST_TYPE dest_type_tmp_var = { __VA_ARGS__ };				\
		memcpy(DEST_ADDR, &dest_type_tmp_var, sizeof(dest_type_tmp_var));	\
	} while (0)

#define NLMSG_ATTR(nlh, hdrlen) ((void *)(nlh) + NLMSG_SPACE(hdrlen))

#endif /* !STRACE_TESTS_H */
