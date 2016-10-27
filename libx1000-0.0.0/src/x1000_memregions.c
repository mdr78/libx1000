#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include "x1000_memregions.h"
#include "x1000_config.h"

static struct dseg *pself_layout = NULL;

static int
read_mapping (FILE *mapfile,
                      long long *addr,
                      long long *endaddr,
                      char *permissions,
                      long long *offset,
                      char *device,
                      long long *inode,
                      char *filename)
{
	int   ret = 0;
	char  fmap[PATH_MAX + 20];

	if(fgets(fmap, PATH_MAX + 20, mapfile) == NULL)
		return EOF;

	ret = sscanf (fmap,  "%llx-%llx %s %llx %s %llx %s",
		addr, endaddr, permissions, offset, device, inode, filename);

	if(ret == 6) filename[0]='\000';

	return ret;
}

static int
free_self_layout () {

	struct  dseg *pnext, *tmp;

	pnext = pself_layout;

	while(pnext)
	{
		tmp=pnext->next;
		free(pnext);
		pnext=tmp;
	}

	pself_layout = NULL;

	return 0;
}

__attribute__((destructor))
static void _free_self_layout() {

	//for neatness sake, free memory on unload.
	free_self_layout ();

}

static int
init_self_layout () {
	FILE *f = 0;
	uint64_t addr, endaddr, offset, inode;
	char	permissions[10];
	char	device[10];
	char	filename[PATH_MAX];
	struct  dseg **pnext; 

	f = fopen("/proc/self/maps", "r");
	if(!f) return -1;

	pnext = &pself_layout;

	while(read_mapping(f, &addr, &endaddr, permissions,
			   &offset, device, &inode, filename) != EOF)
	{
		/*
		 * Find everything with rw permissions
		 */

		if(strcmp(permissions, "rw-p") == 0 )
		{
			//found a data segement
			(*pnext) = (struct dseg *)
				malloc(sizeof(struct dseg));

			(*pnext)->data=addr;
			(*pnext)->size=endaddr-addr;

			if(inode) 
				(*pnext)->type = 1 << X1000_LOCKMEM_DATASEG-1;
			else if(filename[0] == '[')
				(*pnext)->type = 1 << X1000_LOCKMEM_HEAPSTACK-1;
			else
				(*pnext)->type = 1 << X1000_LOCKMEM_MMAP-1;

			(*pnext)->next=NULL;

			pnext = &(*pnext)->next;
		}
	}

	return 0;
}

int get_mlayout(struct dseg **playout)
{
	//free previous queries.
	if(pself_layout)
		free_self_layout();

	if(init_self_layout() < 0)
		return -1;

	*playout = pself_layout;

	return 0;
}
