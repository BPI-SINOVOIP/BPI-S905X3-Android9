/* Release debugging handling context.
   Copyright (C) 2002-2011, 2014 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <search.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "libdwP.h"
#include "cfi.h"


static void
noop_free (void *arg __attribute__ ((unused)))
{
}


static void
cu_free (void *arg)
{
  struct Dwarf_CU *p = (struct Dwarf_CU *) arg;

  Dwarf_Abbrev_Hash_free (&p->abbrev_hash);

  tdestroy (p->locs, noop_free);
}


int
dwarf_end (Dwarf *dwarf)
{
  if (dwarf != NULL)
    {
      if (dwarf->cfi != NULL)
	/* Clean up the CFI cache.  */
	__libdw_destroy_frame_cache (dwarf->cfi);

      Dwarf_Sig8_Hash_free (&dwarf->sig8_hash);

      /* The search tree for the CUs.  NB: the CU data itself is
	 allocated separately, but the abbreviation hash tables need
	 to be handled.  */
      tdestroy (dwarf->cu_tree, cu_free);
      tdestroy (dwarf->tu_tree, cu_free);

      /* Search tree for macro opcode tables.  */
      tdestroy (dwarf->macro_ops, noop_free);

      /* Search tree for decoded .debug_lines units.  */
      tdestroy (dwarf->files_lines, noop_free);

      struct libdw_memblock *memp = dwarf->mem_tail;
      /* The first block is allocated together with the Dwarf object.  */
      while (memp->prev != NULL)
	{
	  struct libdw_memblock *prevp = memp->prev;
	  free (memp);
	  memp = prevp;
	}

      /* Free the pubnames helper structure.  */
      free (dwarf->pubnames_sets);

      /* Free the ELF descriptor if necessary.  */
      if (dwarf->free_elf)
	elf_end (dwarf->elf);

      /* Free the fake location list CU.  */
      if (dwarf->fake_loc_cu != NULL)
	{
	  cu_free (dwarf->fake_loc_cu);
	  free (dwarf->fake_loc_cu);
	}

      /* Free the context descriptor.  */
      free (dwarf);
    }

  return 0;
}
INTDEF(dwarf_end)
