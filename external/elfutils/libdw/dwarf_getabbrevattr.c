/* Get specific attribute of abbreviation.
   Copyright (C) 2003, 2004, 2005, 2014 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <dwarf.h>
#include "libdwP.h"


int
dwarf_getabbrevattr (Dwarf_Abbrev *abbrev, size_t idx, unsigned int *namep,
		     unsigned int *formp, Dwarf_Off *offsetp)
{
  if (abbrev == NULL)
    return -1;

  size_t cnt = 0;
  const unsigned char *attrp = abbrev->attrp;
  const unsigned char *start_attrp;
  unsigned int name;
  unsigned int form;

  do
    {
      start_attrp = attrp;

      /* Attribute code and form are encoded as ULEB128 values.i
         XXX We have no way to bounds check.  */
      get_uleb128 (name, attrp, attrp + len_leb128 (name));
      get_uleb128 (form, attrp, attrp + len_leb128 (form));

      /* If both values are zero the index is out of range.  */
      if (name == 0 && form == 0)
	return -1;
    }
  while (cnt++ < idx);

  /* Store the result if requested.  */
  if (namep != NULL)
    *namep = name;
  if (formp != NULL)
    *formp = form;
  if (offsetp != NULL)
    *offsetp = (start_attrp - abbrev->attrp) + abbrev->offset;

  return 0;
}
