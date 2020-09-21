#ifndef _AIO_H
#define _AIO_H

#include <time.h>
#include <signal.h>
#include <linux/aio_abi.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

struct aiocb {
	int aio_fildes;
	off_t aio_offset;
	volatile void *aio_buf;
	size_t aio_nbytes;
	int aio_reqprio;
	struct sigevent aio_sigevent;
	int aio_lio_opcode;
};

enum {
    AIO_ALLDONE,
    AIO_CANCELED,
    AIO_NOTCANCELED,
};

enum {
    LIO_WAIT,
    LIO_NOWAIT,
};

enum {
    LIO_NOP,
    LIO_READ,
    LIO_WRITE,
};

int aio_read(struct aiocb *);
int aio_write(struct aiocb *);
int aio_fsync(int, struct aiocb *);
int aio_error(const struct aiocb *);
ssize_t aio_return(struct aiocb *);
int aio_suspend(const struct aiocb * const[], int, const struct timespec *);
int aio_cancel(int, struct aiocb *);
int lio_listio(int, struct aiocb *restrict const[restrict], int,
        struct sigevent *restrict);

__END_DECLS

#endif

