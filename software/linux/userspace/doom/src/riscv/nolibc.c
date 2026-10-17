#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>

#include "config.h"

int mini_vsnprintf(char *buffer, unsigned int buffer_len, const char *fmt, va_list va);

extern int   _open(const char *pathname, int flags);
extern int   _close(int fd);
extern ssize_t _read(int fd, void *buf, size_t nbyte);
extern ssize_t _write(int fd, const void *buf, size_t nbyte);
extern off_t _lseek(int fd, off_t offset, int whence);
extern int   _fstat(int fd, void *statbuf);
extern int   _stat(const char *filename, void *statbuf);
extern int   _isatty(int fd);
extern void *_sbrk(intptr_t increment);


static int _errno_val;
int *__errno(void) { return &_errno_val; }

static char _reent_buf[1024] __attribute__((aligned(4)));
void *_impure_ptr = _reent_buf;

#define _U  0x01
#define _L  0x02
#define _N  0x04
#define _S  0x08
#define _P  0x10
#define _C  0x20
#define _X  0x40
#define _B  0x80

const unsigned char _ctype_[257] = {
    0,
    _C,_C,_C,_C,_C,_C,_C,_C,_C,_C|_S|_B,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,
    _C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,_C,
    _S|_B,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,
    _N,_N,_N,_N,_N,_N,_N,_N,_N,_N,_P,_P,_P,_P,_P,_P,
    _P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,_U,_U,_U,_U,_U,_U,_U,_U,
    _U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_P,_P,_P,_P,_P,
    _P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,_L,_L,_L,_L,_L,_L,_L,_L,
    _L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_P,_P,_P,_P,_C,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

#undef _U
#undef _L
#undef _N
#undef _S
#undef _P
#undef _C
#undef _X
#undef _B



void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (((uintptr_t)d & 3) == 0 && ((uintptr_t)s & 3) == 0) {
        uint32_t *d32 = (uint32_t *)d;
        const uint32_t *s32 = (const uint32_t *)s;
        while (n >= 4) {
            *d32++ = *s32++;
            n -= 4;
        }
        d = (uint8_t *)d32;
        s = (const uint8_t *)s32;
    }
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *s, int c, size_t n)
{
    uint8_t *p = s;
    if (((uintptr_t)p & 3) == 0) {
        uint32_t c4 = (uint8_t)c;
        c4 |= c4 << 8;
        c4 |= c4 << 16;
        uint32_t *p32 = (uint32_t *)p;
        while (n >= 4) {
            *p32++ = c4;
            n -= 4;
        }
        p = (uint8_t *)p32;
    }
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p = a;
    const uint8_t *q = b;
    while (n--) {
        unsigned x = *p++ - *q++;
        if (x) return (int)x;
    }
    return 0;
}



size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++)) ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

static int _tolower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

int strcasecmp(const char *a, const char *b)
{
    while (*a && _tolower((unsigned char)*a) == _tolower((unsigned char)*b))
    { a++; b++; }
    return _tolower((unsigned char)*a) - _tolower((unsigned char)*b);
}

int strncasecmp(const char *a, const char *b, size_t n)
{
    while (n && *a && _tolower((unsigned char)*a) == _tolower((unsigned char)*b))
    { a++; b++; n--; }
    return n ? _tolower((unsigned char)*a) - _tolower((unsigned char)*b) : 0;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++)) ;
    return dst;
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}



int abs(int x) { return x < 0 ? -x : x; }



typedef unsigned long long uint64;
typedef long long int64;

uint64 __udivdi3(uint64 num, uint64 den)
{
    uint64 quot = 0, qbit = 1;
    if (den == 0) return 0;
    while ((int64)den >= 0 && den < num) { den <<= 1; qbit <<= 1; }
    while (qbit) {
        if (num >= den) { num -= den; quot += qbit; }
        den >>= 1; qbit >>= 1;
    }
    return quot;
}

int64 __divdi3(int64 num, int64 den)
{
    int neg = 0;
    if (num < 0) { num = -num; neg = !neg; }
    if (den < 0) { den = -den; neg = !neg; }
    uint64 q = __udivdi3((uint64)num, (uint64)den);
    return neg ? -(int64)q : (int64)q;
}

uint64 __umoddi3(uint64 num, uint64 den)
{
    return num - __udivdi3(num, den) * den;
}

int64 __moddi3(int64 num, int64 den)
{
    return num - __divdi3(num, den) * den;
}



struct blk {
    uint32_t size;
    uint32_t free;
    struct blk *next;
};

#define BLK_HDR sizeof(struct blk)
#define ALIGN4(x) (((x) + 3) & ~3)

static struct blk *heap_list = NULL;

void *malloc(size_t size)
{
    if (size == 0) return NULL;
    size = ALIGN4(size);

    struct blk *cur = heap_list;
    while (cur) {
        if (cur->free && cur->size >= size) {
            if (cur->size >= size + BLK_HDR + 8) {
                struct blk *nb = (struct blk *)((uint8_t *)(cur + 1) + size);
                nb->size = cur->size - size - BLK_HDR;
                nb->free = 1;
                nb->next = cur->next;
                cur->size = size;
                cur->next = nb;
            }
            cur->free = 0;
            return (void *)(cur + 1);
        }
        cur = cur->next;
    }

    size_t total = BLK_HDR + size;
    size_t ask = total < 4096 ? 4096 : total;
    struct blk *nb = (struct blk *)_sbrk(ask);
    if ((intptr_t)nb == -1) return NULL;
    nb->size = ask - BLK_HDR;
    nb->free = 0;
    nb->next = NULL;

    if (!heap_list) {
        heap_list = nb;
    } else {
        cur = heap_list;
        while (cur->next) cur = cur->next;
        cur->next = nb;
    }

    if (nb->size >= size + BLK_HDR + 8) {
        struct blk *rem = (struct blk *)((uint8_t *)(nb + 1) + size);
        rem->size = nb->size - size - BLK_HDR;
        rem->free = 1;
        rem->next = nb->next;
        nb->size = size;
        nb->next = rem;
    }

    return (void *)(nb + 1);
}

void free(void *ptr)
{
    if (!ptr) return;
    struct blk *b = (struct blk *)ptr - 1;
    b->free = 1;

    if (b->next && b->next->free) {
        b->size += BLK_HDR + b->next->size;
        b->next = b->next->next;
    }
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    struct blk *b = (struct blk *)ptr - 1;
    if (b->size >= size) return ptr;

    void *np = malloc(size);
    if (np) {
        memcpy(np, ptr, b->size);
        free(ptr);
    }
    return np;
}



int open(const char *pathname, int flags, ...)  { return _open(pathname, flags); }
int close(int fd)                               { return _close(fd); }
ssize_t read(int fd, void *buf, size_t n)      { return _read(fd, buf, n); }
ssize_t write(int fd, const void *buf, size_t n) { return _write(fd, buf, n); }
off_t lseek(int fd, off_t offset, int whence)  { return _lseek(fd, offset, whence); }
int stat(const char *path, void *sb)            { return _stat(path, sb); }
int fstat(int fd, void *sb)                     { return _fstat(fd, sb); }
int isatty(int fd)                              { return _isatty(fd); }




#define __SEOF  0x0020

struct fake_file {
    void  *_p;
    int    _r;
    int    _w;
    short  _flags;
    short  _file;
    char   pad[240];
};

static struct fake_file _ffiles[8] = {
    {NULL, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 1},
    {NULL, 0, 0, 0, 2},
};
static int _ffiles_used = 3;

static int _file_fd(void *fp)
{
    if (!fp) return 1;
    return (int)((struct fake_file *)fp)->_file;
}


static char _printf_buf[256];

int printf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int n = mini_vsnprintf(_printf_buf, sizeof(_printf_buf), fmt, va);
    va_end(va);
    _write(1, _printf_buf, n);
    return n;
}

int fprintf(void *stream, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int n = mini_vsnprintf(_printf_buf, sizeof(_printf_buf), fmt, va);
    va_end(va);
    int fd = _file_fd(stream);
    _write(fd, _printf_buf, n);
    return n;
}

int vfprintf(void *stream, const char *fmt, va_list va)
{
    int n = mini_vsnprintf(_printf_buf, sizeof(_printf_buf), fmt, va);
    int fd = _file_fd(stream);
    _write(fd, _printf_buf, n);
    return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int n = mini_vsnprintf(buf, 0x7fffffff, fmt, va);
    va_end(va);
    return n;
}

int snprintf(char *buf, size_t sz, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int n = mini_vsnprintf(buf, sz, fmt, va);
    va_end(va);
    return n;
}


static int _parse_int(const char **sp, int base)
{
    const char *s = *sp;
    while (*s == ' ' || *s == '\t') s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') { base = 8; }
        else base = 10;
    }
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    int val = 0, got = 0;
    while (*s) {
        int d = -1;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        if (d < 0 || d >= base) break;
        val = val * base + d;
        got = 1;
        s++;
    }
    *sp = s;
    if (!got) return 0;
    return neg ? -val : val;
}

int sscanf(const char *str, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int count = 0;
    const char *s = str;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd' || *fmt == 'i') {
                int *p = va_arg(va, int *);
                *p = _parse_int(&s, (*fmt == 'd') ? 10 : 0);
                count++;
                fmt++;
            } else if (*fmt == 'x') {
                int *p = va_arg(va, int *);
                *p = _parse_int(&s, 16);
                count++;
                fmt++;
            } else if (*fmt == 's') {
                char *p = va_arg(va, char *);
                while (*s == ' ' || *s == '\t') s++;
                while (*s && *s != ' ' && *s != '\t' && *s != '\n')
                    *p++ = *s++;
                *p = '\0';
                count++;
                fmt++;
            } else if (*fmt == 'c') {
                char *p = va_arg(va, char *);
                *p = *s++;
                count++;
                fmt++;
            } else {
                fmt++;
            }
        } else if (*fmt == ' ') {
            while (*s == ' ' || *s == '\t') s++;
            fmt++;
        } else {
            if (*s == *fmt) s++;
            fmt++;
        }
    }

    va_end(va);
    return count;
}

int fscanf(void *stream, const char *fmt, ...)
{
    (void)stream;
    (void)fmt;
    return 0;
}


void *fopen(const char *path, const char *mode)
{
    (void)mode;
    if (!path || strcmp(path, "doom1.wad") != 0)
        return NULL;
    int fd = _open(path, 0);
    if (fd < 0) return NULL;

    if (_ffiles_used >= 8) { _close(fd); return NULL; }
    int idx = _ffiles_used++;
    struct fake_file *f = &_ffiles[idx];
    f->_p     = NULL;
    f->_r     = 0;
    f->_w     = 0;
    f->_flags = __SEOF;
    f->_file  = (short)fd;

    return f;
}

int fclose(void *stream)
{
    if (!stream) return -1;
    int fd = _file_fd(stream);
    return _close(fd);
}

int fflush(void *stream)
{
    (void)stream;
    return 0;
}

int feof(void *stream)
{
    if (!stream) return 1;
    return (((struct fake_file *)stream)->_flags & __SEOF) ? 1 : 0;
}

void setbuf(void *stream, char *buf)
{
    (void)stream;
    (void)buf;
}



void exit(int status)
{
    (void)status;
    while (1) ;
}



int atoi(const char *s)
{
    const char *p = s;
    return _parse_int(&p, 10);
}

long strtol(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    long val = _parse_int(&s, base);
    if (endptr) *endptr = (char *)s;
    return val;
}

char *getenv(const char *name)
{
    (void)name;
    return NULL;
}
