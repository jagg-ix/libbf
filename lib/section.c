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

#include "section.h"

#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libelf.h>

void section_init(struct section *s, const char *name, int idx, Elf_Shdr shdr) {
  s->name = name;
  s->idx = idx;
  s->shdr = shdr; // TODO: should we use mmalloc + memcpy

  INIT_LIST_HEAD(&s->sections);
  INIT_LIST_HEAD(&s->segment_entry);
  INIT_HLIST_NODE(&s->section_hash);
}

struct section *section_find(struct section_table *table, const char *name) {
  struct hlist_head *head;
  struct hlist_node *node;
  struct section *s;

  head = &table->section_hash[section_hashfn(name)];
  hlist_for_each_entry(s, node, head, section_hash) {
    if (strcmp(s->name, name) == 0)
      return s;
  }

  return NULL;
}

void section_add(struct section_table *table, struct section *scn) {
  struct hlist_head *head;
  struct hlist_node *node;
  struct section *s;

  head = &table->section_hash[section_hashfn(scn->name)];
  hlist_for_each_entry(s, node, head, section_hash) {
    if (strcmp(s->name, scn->name) == 0)
      return;
  }

  hlist_add_head(&scn->section_hash, head);
}

void section_table_init(struct section_table *table) {
  table->section_hash = malloc(sizeof(struct hlist_head) * sectionhash_size);
  for (int i = 0; i < sectionhash_size; i++)
    INIT_HLIST_HEAD(&table->section_hash[i]);
}

void section_table_destroy(struct section_table *table) {
  if (!table)
    return;

  free(table->section_hash);
}
