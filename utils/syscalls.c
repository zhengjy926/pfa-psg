/*
 * Minimal newlib syscall stubs for the bare-metal image.
 *
 * The firmware never performs file or heap I/O; these exist so the linker
 * resolves newlib references without the "not implemented" warnings from
 * nosys.specs.  N-001 forbids dynamic allocation, so _sbrk fails.
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

int _write(int file, char *ptr, int len);
int _read(int file, char *ptr, int len);
int _close(int file);
int _lseek(int file, int ptr, int dir);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _getpid(void);
int _kill(int pid, int sig);
void *_sbrk(intptr_t increment);

int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

int _close(int file)
{
    (void)file;
    errno = ENOSYS;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    errno = ENOSYS;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

void *_sbrk(intptr_t increment)
{
    (void)increment;
    errno = ENOMEM;
    return (void *)-1;
}
