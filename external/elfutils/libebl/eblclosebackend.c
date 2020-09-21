/* Free ELF backend handle.
   Copyright (C) 2000, 2001, 2002 Red Hat, Inc.
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

#include <dlfcn.h>
#include <stdlib.h>

#include <libeblP.h>


void
ebl_closebackend (Ebl *ebl)
{
  if (ebl != NULL)
    {
      /* Run the destructor.  */
      ebl->destr (ebl);

      /* Close the dynamically loaded object.  */
      if (ebl->dlhandle != NULL)
	(void) dlclose (ebl->dlhandle);

      /* Free the resources.  */
      free (ebl);
    }
}
