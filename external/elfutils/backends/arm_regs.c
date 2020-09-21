/* Register names and numbers for ARM DWARF.
   Copyright (C) 2009 Red Hat, Inc.
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

#include <string.h>
#include <dwarf.h>

#define BACKEND arm_
#include "libebl_CPU.h"

ssize_t
arm_register_info (Ebl *ebl __attribute__ ((unused)),
		   int regno, char *name, size_t namelen,
		   const char **prefix, const char **setname,
		   int *bits, int *type)
{
  if (name == NULL)
    return 320;

  if (regno < 0 || regno > 320 || namelen < 5)
    return -1;

  *prefix = "";
  *bits = 32;
  *type = DW_ATE_signed;
  *setname = "integer";

  switch (regno)
    {
    case 0 ... 9:
      name[0] = 'r';
      name[1] = regno + '0';
      namelen = 2;
      break;

    case 10 ... 12:
      name[0] = 'r';
      name[1] = '1';
      name[2] = regno % 10 + '0';
      namelen = 3;
      break;

    case 13 ... 15:
      *type = DW_ATE_address;
      name[0] = "slp"[regno - 13];
      name[1] = "prc"[regno - 13];
      namelen = 2;
      break;

    case 16 + 0 ... 16 + 7:
      regno += 96 - 16;
      /* Fall through.  */
    case 96 + 0 ... 96 + 7:
      *setname = "FPA";
      *type = DW_ATE_float;
      *bits = 96;
      name[0] = 'f';
      name[1] = regno - 96 + '0';
      namelen = 2;
      break;

    case 128:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "spsr") + 1 - name;

    case 256 + 0 ... 256 + 9:
      *setname = "VFP";
      *type = DW_ATE_float;
      *bits = 64;
      name[0] = 'd';
      name[1] = regno - 256 + '0';
      namelen = 2;
      break;

    case 256 + 10 ... 256 + 31:
      *setname = "VFP";
      *type = DW_ATE_float;
      *bits = 64;
      name[0] = 'd';
      name[1] = (regno - 256) / 10 + '0';
      name[2] = (regno - 256) % 10 + '0';
      namelen = 3;
      break;

    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
