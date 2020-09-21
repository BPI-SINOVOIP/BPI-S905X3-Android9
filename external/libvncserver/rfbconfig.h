/* rfbconfig.h.  Generated from rfbconfig.h.in by configure.  */
/* rfbconfig.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Enable 24 bit per pixel in native framebuffer */
#define ALLOW24BPP 1

/* work around when write() returns ENOENT but does not mean it */
/* #undef ENOENT_WORKAROUND */

/* Use ffmpeg (for vnc2mpg) */
/* #undef FFMPEG */

/* Android host system detected */
/* #undef HAVE_ANDROID */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Avahi/mDNS client build environment present */
/* #undef HAVE_AVAHI */

/* Define to 1 if you have the `crypt' function. */
/* #undef HAVE_CRYPT */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* DPMS extension build environment present */
/* #undef HAVE_DPMS */

/* FBPM extension build environment present */
/* #undef HAVE_FBPM */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `ftime' function. */
#define HAVE_FTIME 1

/* Define to 1 if you have the `geteuid' function. */
/* #undef HAVE_GETEUID */

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getpwnam' function. */
/* #undef HAVE_GETPWNAM */

/* Define to 1 if you have the `getpwuid' function. */
/* #undef HAVE_GETPWUID */

/* Define to 1 if you have the `getspnam' function. */
/* #undef HAVE_GETSPNAM */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `getuid' function. */
/* #undef HAVE_GETUID */

/* GnuTLS library present */
/* #undef HAVE_GNUTLS */

/* Define to 1 if you have the `grantpt' function. */
/* #undef HAVE_GRANTPT */

/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1

/* Define to 1 if you have the `initgroups' function. */
/* #undef HAVE_INITGROUPS */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* IRIX XReadDisplay available */
/* #undef HAVE_IRIX_XREADDISPLAY */

/* libcrypt library present */
#define HAVE_LIBCRYPT 1

/* openssl libcrypto library present */
/* #undef HAVE_LIBCRYPTO */

/* Define to 1 if you have the `cygipc' library (-lcygipc). */
/* #undef HAVE_LIBCYGIPC */

/* libjpeg support enabled */
#define HAVE_LIBJPEG 1

/* Define to 1 if you have the `nsl' library (-lnsl). */
#define HAVE_LIBNSL 1

/* Define to 1 if you have the `png' library (-lpng). */
#define HAVE_LIBPNG 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* openssl libssl library present */
/* #undef HAVE_LIBSSL */

/* XDAMAGE extension build environment present */
/* #undef HAVE_LIBXDAMAGE */

/* XFIXES extension build environment present */
/* #undef HAVE_LIBXFIXES */

/* XINERAMA extension build environment present */
/* #undef HAVE_LIBXINERAMA */

/* XRANDR extension build environment present */
/* #undef HAVE_LIBXRANDR */

/* DEC-XTRAP extension build environment present */
/* #undef HAVE_LIBXTRAP */

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* linux fb device build environment present */
/* #undef HAVE_LINUX_FB_H */

/* linux/input.h present */
/* #undef HAVE_LINUX_INPUT_H */

/* linux uinput device build environment present */
/* #undef HAVE_LINUX_UINPUT_H */

/* video4linux build environment present */
/* #undef HAVE_LINUX_VIDEODEV_H */

/* build MacOS X native display support */
/* #undef HAVE_MACOSX_NATIVE_DISPLAY */

/* MacOS X OpenGL present */
/* #undef HAVE_MACOSX_OPENGL_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkfifo' function. */
#define HAVE_MKFIFO 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <pwd.h> header file. */
/* #undef HAVE_PWD_H */

/* RECORD extension build environment present */
/* #undef HAVE_RECORD */

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setegid' function. */
/* #undef HAVE_SETEGID */

/* Define to 1 if you have the `seteuid' function. */
/* #undef HAVE_SETEUID */

/* Define to 1 if you have the `setgid' function. */
/* #undef HAVE_SETGID */

/* Define to 1 if you have the `setpgrp' function. */
/* #undef HAVE_SETPGRP */

/* Define to 1 if you have the `setsid' function. */
/* #undef HAVE_SETSID */

/* Define to 1 if you have the `setuid' function. */
/* #undef HAVE_SETUID */

/* Define to 1 if you have the `setutxent' function. */
/* #undef HAVE_SETUTXENT */

/* Define to 1 if you have the `shmat' function. */
/* #undef HAVE_SHMAT */

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Solaris XReadScreen available */
/* #undef HAVE_SOLARIS_XREADSCREEN */

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strcspn' function. */
#define HAVE_STRCSPN 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Use the system libvncserver build environment for x11vnc. */
/* #undef HAVE_SYSTEM_LIBVNCSERVER */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/stropts.h> header file. */
/* #undef HAVE_SYS_STROPTS_H */

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
/* #undef HAVE_TERMIOS_H */

/* Define to 1 if compiler supports __thread */
#define HAVE_TLS 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <utmpx.h> header file. */
/* #undef HAVE_UTMPX_H */

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `waitpid' function. */
/* #undef HAVE_WAITPID */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
/* #undef HAVE_WS2TCPIP_H */

/* X11 build environment present */
#define HAVE_X11 1

/* open ssl X509_print_ex_fp available */
/* #undef HAVE_X509_PRINT_EX_FP */

/* XKEYBOARD extension build environment present */
/* #undef HAVE_XKEYBOARD */

/* MIT-SHM extension build environment present */
/* #undef HAVE_XSHM */

/* XTEST extension build environment present */
/* #undef HAVE_XTEST */

/* XTEST extension has XTestGrabControl */
/* #undef HAVE_XTESTGRABCONTROL */

/* Enable IPv6 support */
#define IPv6 1

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1

/* Need a typedef for in_addr_t */
/* #undef NEED_INADDR_T */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "LibVNCServer"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://sourceforge.net/projects/libvncserver"

/* Define to the full name of this package. */
#define PACKAGE_NAME "LibVNCServer"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "LibVNCServer 0.9.9"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libvncserver"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.9.9"

/* The number of bytes in type char */
/* #undef SIZEOF_CHAR */

/* The number of bytes in type int */
/* #undef SIZEOF_INT */

/* The number of bytes in type long */
/* #undef SIZEOF_LONG */

/* The number of bytes in type short */
/* #undef SIZEOF_SHORT */

/* The number of bytes in type void* */
/* #undef SIZEOF_VOIDP */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "0.9.9"

/* Enable support for libgcrypt in libvncclient */
/* #undef WITH_CLIENT_GCRYPT */

/* Disable TightVNCFileTransfer protocol */
#define WITH_TIGHTVNC_FILETRANSFER 1

/* Disable WebSockets support */
#define WITH_WEBSOCKETS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* The type for socklen */
/* #undef socklen_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
