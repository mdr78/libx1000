
#define X1000_LOCKMEM_DATASEG	1
#define X1000_LOCKMEM_HEAPSTACK 2
#define X1000_LOCKMEM_MMAP	3
#define X1000_LOCKMEM_MAX	4
#define X1000_EXECHOOK_PREINIT	1
#define X1000_EXECHOOK_FORK	2
#define X1000_EXECHOOK_MMAP	3
#define X1000_EXECHOOK_MAX	4
#define X1000_SECTION_MAX	2

uint32_t get_config(uint32_t *lockmem, uint32_t *phase);
