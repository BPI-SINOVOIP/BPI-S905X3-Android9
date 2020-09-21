/* Describe known core note formats.
   Copyright (C) 2007, 2010 Red Hat, Inc.
   This file is part of elfutils.

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

#include <assert.h>
#include <byteswap.h>
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <libeblP.h>


int
ebl_core_note (Ebl *ebl, const GElf_Nhdr *nhdr, const char *name,
	       GElf_Word *regs_offset, size_t *nregloc,
	       const Ebl_Register_Location **reglocs, size_t *nitems,
	       const Ebl_Core_Item **items)
{
  int result = ebl->core_note (nhdr, name,
			       regs_offset, nregloc, reglocs, nitems, items);
  if (result == 0)
    {
      /* The machine specific function did not know this type.  */

      *regs_offset = 0;
      *nregloc = 0;
      *reglocs = NULL;
      switch (nhdr->n_type)
	{
#define ITEMS(type, table)				\
	  case type:					\
	    *items = table;				\
	    *nitems = sizeof table / sizeof table[0];	\
	    result = 1;					\
	    break

	  static const Ebl_Core_Item platform[] =
	    {
	      {
		.name = "Platform",
		.type = ELF_T_BYTE, .count = 0, .format = 's'
	      }
	    };
	  ITEMS (NT_PLATFORM, platform);

#undef	ITEMS
	}
    }

  return result;
}
