#include <stdint.h>
#include <stdio.h>
#include <x1000_memregions.h>

#define TO_PTR(x) (void *)(uint32_t)(x & 0xFFFFFFFFULL)

int main(int argc, void *argv)
{
	struct dseg *next;

	if(get_mlayout(&next) < 0)
		printf("Failed to get memory layout\n");

	while(next)
	{
		printf("Data,Type,Size: %p,%lu,%llu\n", 
		       TO_PTR(next->data),next->type, next->size);
		next = next->next;
	}

	return 0;
}
