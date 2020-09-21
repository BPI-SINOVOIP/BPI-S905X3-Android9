/* Return one of the sources lines of a CU.
   Copyright (C) 2005 Red Hat, Inc.
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

#include "libdwflP.h"

Dwfl_Line *
dwfl_onesrcline (Dwarf_Die *cudie, size_t idx)
{
  struct dwfl_cu *cu = (struct dwfl_cu *) cudie;

  if (cudie == NULL)
    return NULL;

  if (cu->lines == NULL)
    {
      Dwfl_Error error = __libdwfl_cu_getsrclines (cu);
      if (error != DWFL_E_NOERROR)
	{
	  __libdwfl_seterrno (error);
	  return NULL;
	}
    }

  if (idx >= cu->die.cu->lines->nlines)
    {
      __libdwfl_seterrno (DWFL_E (LIBDW, DWARF_E_INVALID_LINE_IDX));
      return NULL;
    }

  return &cu->lines->idx[idx];
}
