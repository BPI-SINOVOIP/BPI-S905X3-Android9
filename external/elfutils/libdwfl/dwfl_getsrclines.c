/* Fetch source line information for CU.
   Copyright (C) 2005, 2013 Red Hat, Inc.
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

int
dwfl_getsrclines (Dwarf_Die *cudie, size_t *nlines)
{
  struct dwfl_cu *cu = (struct dwfl_cu *) cudie;

  if (cu->lines == NULL)
    {
      Dwfl_Error error = __libdwfl_cu_getsrclines (cu);
      if (error != DWFL_E_NOERROR)
	{
	  __libdwfl_seterrno (error);
	  return -1;
	}
    }

  *nlines = cu->die.cu->lines->nlines;
  return 0;
}
