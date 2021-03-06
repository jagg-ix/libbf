/*
 * This file is part of libbf.
 *
 * libbf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libbf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with libbf.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mem_manager.h"

/*
 * Method of locating section from VMA borrowed from opdis.
 */
typedef struct {
	bfd_vma    vma;
	asection * sec;
} BFD_VMA_SECTION;

/*
 * unload_all_sections should be called when sections are no longer needed.
 */
static struct bf_mem_block * load_section(asection * s)
{
	struct bf_mem_block * mem    = xmalloc(sizeof(struct bf_mem_block));
	int		      size   = bfd_section_size(s->owner, s);
	unsigned char *	      buffer = xmalloc(size);

	if(!bfd_get_section_contents(s->owner, s, buffer, 0, size)) {
		free(buffer);
		free(mem);
		return NULL;
	}

	mem->section	   = s;
	mem->buffer	   = buffer;
	mem->buffer_length = size;
	mem->buffer_vma	   = bfd_get_section_vma(s->owner, s);
	return mem;
}

static void vma_in_section(bfd * abfd, asection * s, void * data)
{
	BFD_VMA_SECTION * req = data;

	if(req && req->vma >= s->vma &&
			req->vma < (s->vma + bfd_section_size(abfd, s)) ) {
		req->sec = s;
	}
}

static asection * section_from_vma(struct bin_file * bf,
		bfd_vma vma)
{
	BFD_VMA_SECTION req = {vma, NULL};
	bfd_map_over_sections(bf->abfd, vma_in_section, &req);

	if(!req.sec) {
		printf("Failed to locate section: 0x%lX\n", vma);
		return NULL;
	}

	return req.sec;
}

struct bf_mem_block * load_section_for_vma(struct bin_file * bf,
		bfd_vma vma)
{
	asection * s	      = section_from_vma(bf, vma);
	bfd_vma	   buffer_vma = bfd_get_section_vma(s->owner, s);

	struct htable_entry * entry = htable_find(&bf->mem_table,
			&buffer_vma, sizeof(buffer_vma));

	if(entry != NULL) {
		return hash_entry(entry, struct bf_mem_block, entry);
	} else {
		struct bf_mem_block * mem = load_section(s);

		if(!mem) {
			printf("Failed to load section: 0x%lX\n", vma);
			return NULL;
		} else {
			htable_add(&bf->mem_table, &mem->entry,
					&mem->buffer_vma,
					sizeof(mem->buffer_vma));
		}

		return mem;
	}
}

void unload_all_sections(struct bin_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_mem_block * mem;

	htable_for_each_entry_safe(mem, cur_entry, n, &bf->mem_table, entry) {
		htable_del_entry(&bf->mem_table, cur_entry);
		free(mem->buffer);
		free(mem);
	}
}

