#include <sys/mman.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include "x1000_memregions.h"
#include "x1000_config.h"

/*
 * A conservative memory prefault for Intel X1000.
 * 1. Hook the start of execution with a constructor.
 * 2. Find heap and data segment addresses, and prefault
 * 3. Hook forks and prefault heap and data segements.
 * 4. Catch any attempts to resize the heap via mresize, prefault the heap. 
 * 5. Never using MCL_FUTURE means sparse files should be ok.
 * 6. Only gap is locks in shared memory, created at runtime.
 *
 */

#define TO_BITMASK(x) (1 << (x-1))

static uint32_t	exechook = 0;
static uint32_t memlock = 0;
static pid_t (*lfork)(void) = NULL;

static void * (*lmremap) (void *start, size_t old_len, size_t len, int flags,
			void *newaddr) = NULL;
static void * (*lmmap)(void *addr, size_t length, int prot, int flags,
		        int fd, off_t offset) = 0;
static void * (*lmmap64)(void *addr, size_t length, int prot, int flags,
	                int fd, __off64_t offset) = 0;


#define TO_UINT32(x) (uint32_t)(x & 0xFFFFFFFFULL)
#define TO_PTR(x) (void *) TO_UINT32(x)

/*
 * Implementation of a conservative memory lock strategy, only locking 
 * data segements and heap into memory. 
 */

static void x1000_memlock(uint32_t memlock)
{
	struct dseg *next;

	if(get_mlayout(&next) < 0)
		return;

	//for each data segment found, prefault it
        while(next)
	{
		if(memlock & next->type)
			mlock(TO_PTR(next->data), TO_UINT32(next->size));

		next = next->next;
	}
}

__attribute__((constructor))
static void libx1000_ldpreload_init() {

	memlock = exechook = 0;

	lfork = dlsym(RTLD_NEXT, "fork");
	lmremap = dlsym(RTLD_NEXT, "mremap");
	lmmap64 = dlsym(RTLD_NEXT, "mmap64");
	lmmap = dlsym(RTLD_NEXT, "mmap");

	get_config(&memlock, &exechook);

	if(exechook & TO_BITMASK(X1000_EXECHOOK_PREINIT))
		x1000_memlock(memlock);
}

void *
mremap (void *start, size_t old_len, size_t len, int flags, ...)
{
	va_list ap;

	va_start (ap, flags);
	void *newaddr = (flags & MREMAP_FIXED) ? va_arg (ap, void *) : NULL;
	va_end (ap);

	if(exechook & TO_BITMASK(X1000_EXECHOOK_MMAP) &&
		memlock & TO_BITMASK(X1000_LOCKMEM_MMAP))
		flags &= MAP_POPULATE;

remap_err:
	return lmremap(start, old_len, len, flags, newaddr);
}

void *mmap64(void *addr, size_t length, int prot, int flags,
                    int fd, __off64_t offset)

{
	if(exechook & TO_BITMASK(X1000_EXECHOOK_MMAP) &&
                memlock & TO_BITMASK(X1000_LOCKMEM_MMAP))
	{
                flags |= MAP_POPULATE;
	}

	return lmmap64(addr, length, prot, flags, fd, offset);
}

void *mmap(void *addr, size_t length, int prot, int flags,
            int fd, off_t offset) 
{
	if(exechook & TO_BITMASK(X1000_EXECHOOK_MMAP) &&
                memlock & TO_BITMASK(X1000_LOCKMEM_MMAP))
	{
                flags |= MAP_POPULATE;
	}

	return lmmap(addr, length, prot, flags, fd, offset);
}

pid_t fork(void)
{
	pid_t retval = lfork();

	if(exechook & TO_BITMASK(X1000_EXECHOOK_FORK))
		x1000_memlock(memlock);

	return retval;
}
