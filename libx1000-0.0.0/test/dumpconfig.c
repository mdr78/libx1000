#include <stdint.h>
#include <stdio.h>
#include <x1000_config.h>

int main(int argc, void *argv)
{
	uint32_t exechook,lockmem;

	if(get_config(&exechook, &lockmem) < 0)
		printf("Failed to get configuration\n");

	printf("ExecHook,Lockmem: %lu,%lu\n", exechook, lockmem);

	return 0;
}
