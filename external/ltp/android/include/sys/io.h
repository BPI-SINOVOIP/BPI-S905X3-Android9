#include <sys/syscall.h>
#include <asm/unistd.h>

#ifdef __NR_iopl
static inline int iopl(int level)
{
    return syscall(__NR_iopl, level);
}
#endif /* __NR_iopl */

#ifdef __NR_ioperm
static inline int ioperm(unsigned long from, unsigned long num, int turn_on)
{
    return syscall(__NR_ioperm, from, num, turn_on);
}
#endif /* __NR_ioperm */
