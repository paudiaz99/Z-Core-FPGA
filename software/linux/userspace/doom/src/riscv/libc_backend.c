#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

#include "config.h"
#include "console.h"


#define LIBC_DEBUG



extern uint8_t _heap_start;
extern uint8_t __heap_limit;
static void *heap_end   = &_heap_start;

void *
_sbrk(intptr_t increment)
{
	void *rv = heap_end;
	if (increment > 0)
		increment = (increment + 3) & ~(intptr_t)3;

	void *new_end = (uint8_t *)heap_end + increment;

	if ((uint8_t *)new_end >= &__heap_limit) {
		console_puts("_sbrk: HEAP OVERFLOW!\r\n");
		return (void *)-1;
	}

	heap_end = new_end;
#ifdef LIBC_DEBUG
	console_printf("Heap extended to %08x\r\n", (uint32_t)(uintptr_t)heap_end);
#endif
	return rv;
}



static struct {
	const char *name;
	size_t      len;
	void *      addr;
} fs[] = {
	{ "doom1.wad", WAD_SIZE, (void*)WAD_BASE },
	{ NULL }
};


#define NUM_FDS		16

static struct {
	enum {
		FD_NONE  = 0,
		FD_STDIO = 1,
		FD_FLASH = 2,
	} type;
	size_t offset;
	size_t len;
	void   *data;
} fds[NUM_FDS] = {
	[0] = {
		.type = FD_STDIO,
	},
	[1] = {
		.type = FD_STDIO,
	},
	[2] = {
		.type = FD_STDIO,
	},
};

int
_open(const char *pathname, int flags)
{
	int fn, fd;

	(void)flags;

	for (fn=0; fs[fn].name; fn++)
		if (!strcmp(pathname, fs[fn].name))
			break;

	if (!fs[fn].name) {
		errno = ENOENT;
		return -1;
	}

	if (strcmp(pathname, "doom1.wad") != 0) {
		errno = ENOENT;
		return -1;
	}

	for (fd=3; (fd<NUM_FDS) && (fds[fd].type != FD_NONE); fd++);
	if (fd == NUM_FDS) {
		errno = ENOMEM;
		return -1;
	}

	fds[fd].type   = FD_FLASH;
	fds[fd].offset = 0;
	fds[fd].len    = fs[fn].len;
	fds[fd].data   = fs[fn].addr;

	console_printf("Opened: %s as fd=%d\n", pathname, fd);
#ifdef ZCORE_DOOM
	console_printf("[UART] _open: %s len=%u base=0x%08x\r\n",
		       pathname, (unsigned)fs[fn].len,
		       (unsigned)(size_t)fs[fn].addr);
#endif

	return fd;
}

ssize_t
_read(int fd, void *buf, size_t nbyte)
{
	if ((fd < 0) || (fd >= NUM_FDS) || (fds[fd].type != FD_FLASH)) {
		errno = EINVAL;
		return -1;
	}

	if ((fds[fd].offset + nbyte) > fds[fd].len)
		nbyte = fds[fd].len - fds[fd].offset;

	memcpy(buf, fds[fd].data + fds[fd].offset, nbyte);
	fds[fd].offset += nbyte;

	return nbyte;
}

ssize_t
_write(int fd, const void *buf, size_t nbyte)
{
	const unsigned char *c = buf;
	for (int i=0; i<nbyte; i++)
		console_putchar(*c++);
	return nbyte;
}

int
_close(int fd)
{
	if ((fd < 0) || (fd >= NUM_FDS)) {
		errno = EINVAL;
		return -1;
	}

	fds[fd].type = FD_NONE;

	return 0;
}

off_t
_lseek(int fd, off_t offset, int whence)
{
	off_t newpos;

	if ((fd < 0) || (fd >= NUM_FDS) || (fds[fd].type != FD_FLASH)) {
		errno = EINVAL;
		return -1;
	}

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = (off_t)fds[fd].offset + offset;
		break;
	case SEEK_END:
		newpos = (off_t)fds[fd].len + offset;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (newpos < 0 || newpos > (off_t)fds[fd].len) {
		errno = EINVAL;
		return -1;
	}

	fds[fd].offset = (size_t)newpos;

	return newpos;
}

int
_stat(const char *filename, struct stat *statbuf)
{
#ifdef LIBC_DEBUG
	console_printf("[1] Unimplemented _stat(filename=\"%s\")\n", filename);
#endif

	return -1;
}

int
_fstat(int fd, struct stat *statbuf)
{
	if ((fd < 0) || (fd >= NUM_FDS)) {
		errno = EINVAL;
		return -1;
	}

	memset(statbuf, 0, sizeof(*statbuf));

	if (fds[fd].type == FD_STDIO) {
		statbuf->st_mode = 0020000;
		return 0;
	}

	if (fds[fd].type == FD_FLASH) {
		statbuf->st_mode = 0100000;
		statbuf->st_size = fds[fd].len;
		return 0;
	}

	errno = EBADF;
	return -1;
}

int
_isatty(int fd)
{
	errno = 0;
	return (fd == 1) || (fd == 2);
}

int
access(const char *pathname, int mode)
{
	int fn;

	for (fn=0; fs[fn].name; fn++)
		if (!strcmp(pathname, fs[fn].name))
			break;

	if (!fs[fn].name) {
		errno = ENOENT;
		return -1;
	}

	if (mode & ~(R_OK | F_OK)) {
		errno = EACCES;
		return -1;
	}

	return 0;
}


int
_kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	errno = EINVAL;
	return -1;
}

int
_getpid(void)
{
	return 1;
}
