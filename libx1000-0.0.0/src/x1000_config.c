#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include "x1000_config.h"

struct _configtoken
{
	const char *token;
	uint8_t bit;
};

const struct _configtoken lockmem_tokens[X1000_LOCKMEM_MAX] =
{
	{"DS", X1000_LOCKMEM_DATASEG},
	{"HS", X1000_LOCKMEM_HEAPSTACK},
	{"MM", X1000_LOCKMEM_MMAP}
};

const struct _configtoken exechook_tokens[X1000_EXECHOOK_MAX] =
{
        {"PI", X1000_EXECHOOK_PREINIT},
        {"FK", X1000_EXECHOOK_FORK},
        {"MM", X1000_EXECHOOK_MMAP}
};

struct _configsection
{
        const char *section;
        typeof(lockmem_tokens) *ptokens;
};

const struct _configsection sections[X1000_SECTION_MAX] =
{
	{"MEMLOCK", &lockmem_tokens},
	{"EXECHOOK", &exechook_tokens}
};

static const char *pconfigdirs[] =
		{"/etc/libx1000.so.d/",
		 "/proc/self/cwd/",
		 NULL};

static FILE *open_config_file()
{
	char linkat[PATH_MAX];
	char configname[PATH_MAX];
	char *pfilename[3] = {NULL, "defaults", NULL};
	FILE *f;
	uint32_t i=0;
	uint32_t e=0;

	if(readlink("/proc/self/exe", linkat, PATH_MAX) < 0)
                return NULL; 

        pfilename[0] = basename(linkat);

	while(pfilename[e])
	{
		while(pconfigdirs[i])
		{
		        sprintf(configname, "%s%s.conf", 
				pconfigdirs[i], pfilename[e]);

		        f = fopen(configname, "r");
			if(f) return f;

			i++;
		}

		i=0; e++;
	}

	return NULL;
}

static  uint32_t find_section(const char *psection)
{
	uint32_t i;

	for(i=0;i<X1000_SECTION_MAX;i++)
		if(strcmp(sections[i].section, psection) == 0)
			return i;

	return X1000_SECTION_MAX;
}

static  uint32_t parse_tokens(uint32_t section, char *pconfig_tokens)
{
	char *pstoken;
	typeof(lockmem_tokens) *ptokens = sections[section].ptokens;
	uint32_t i;
	uint32_t bitmask = 0;

	pstoken = strtok(pconfig_tokens,"|");

	while(pstoken)
	{
		for(i=0;i<X1000_LOCKMEM_MAX;i++)
		{
			if(strcmp(pstoken, (*ptokens)[i].token) == 0)
			{
				bitmask |= 1 << (*ptokens)[i].bit-1;
				break;
			}
		}

		pstoken = strtok(NULL,"|");
	}

	return bitmask;
}

uint32_t get_config(uint32_t *plockmem, uint32_t *pphase)
{
	char buffer[PATH_MAX];
	char *psection, *ptokens;
	uint32_t section_index, tokens;

	char *ptoken;
	FILE *f;

	uint32_t *r[X1000_SECTION_MAX] = {plockmem, pphase};

	*plockmem = *pphase = 0;

	f = open_config_file();
	if(!f) return 0;

	while(fgets(buffer,PATH_MAX,f))
	{
		//ignore comments
		if(buffer[0]=='#' || buffer[0]=='\0') continue;

		//parse strings or ignore
		if(sscanf(buffer, "%m[^'=']=%ms", &psection, &ptokens) < 2) continue;

		section_index = find_section(psection);
		if(section_index != X1000_SECTION_MAX)
			*r[section_index] = parse_tokens(section_index, ptokens);

		free(psection);
		free(ptokens);
	}

	fclose(f);

	return (*plockmem > 0 && *pphase > 0);
}
