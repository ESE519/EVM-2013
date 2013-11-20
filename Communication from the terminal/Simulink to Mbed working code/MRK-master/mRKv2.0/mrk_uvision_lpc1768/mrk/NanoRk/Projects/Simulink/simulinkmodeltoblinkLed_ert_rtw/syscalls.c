/*
 * ARM Cortex-M system calls
 *
 * Copyright 2013 The MathWorks, Inc.
 */
#include <sys/stat.h>
 

int _close(int file) 
{ 
    return -1; 
}

int _fstat(int file, struct stat *st) 
{
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) 
{ 
    return 1; 
}

int _lseek(int file, int ptr, int dir) 
{ 
    return 0; 
}

int _open(const char *name, int flags, int mode) 
{ 
    return -1; 
}

int _read(int file, char *ptr, int len) 
{
    return 0;
}

void _exit(int status)
{
}

int _fini(void)
{
    return -1;
}

/* Used for malloc() */
extern unsigned int _sheap;  /* Start of HEAP */
extern unsigned int _eheap;  /* End of HEAP  */
caddr_t heap_end = 0;
caddr_t _sbrk(int incr) {
    caddr_t prev_heap_end;
    caddr_t tmp;
    
    if (heap_end == 0) {
        heap_end = (caddr_t) &_sheap;
    }
    prev_heap_end = heap_end;
    
    /* Always align to an eight byte boundary */
    tmp = (caddr_t) (((unsigned int)heap_end + incr + 7) & ~7);
    if (tmp >= ((caddr_t)&_eheap)) {
        /* No more HEAP */
        return (caddr_t)0;
    }
    heap_end = tmp;
    
    return prev_heap_end;
}

int _write(int file, char *ptr, int len) 
{
    return len;
}