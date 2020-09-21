/* Get attributes of the DIE.
   Copyright (C) 2004, 2005, 2008, 2009, 2014 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#include "libdwP.h"


ptrdiff_t
dwarf_getattrs (Dwarf_Die *die, int (*callback) (Dwarf_Attribute *, void *),
		void *arg, ptrdiff_t offset)
{
  if (die == NULL)
    return -1l;

  if (unlikely (offset == 1))
    return 1;

  const unsigned char *die_addr;

  /* Find the abbreviation entry.  */
  Dwarf_Abbrev *abbrevp = __libdw_dieabbrev (die, &die_addr);

  if (unlikely (abbrevp == DWARF_END_ABBREV))
    {
    invalid_dwarf:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1l;
    }

  /* This is where the attributes start.  */
  const unsigned char *attrp = abbrevp->attrp;
  const unsigned char *const offset_attrp = abbrevp->attrp + offset;

  /* Go over the list of attributes.  */
  Dwarf *dbg = die->cu->dbg;
  const unsigned char *endp;
  endp = ((const unsigned char *) dbg->sectiondata[IDX_debug_abbrev]->d_buf
	  + dbg->sectiondata[IDX_debug_abbrev]->d_size);
  while (1)
    {
      /* Are we still in bounds?  */
      if (unlikely (attrp >= endp))
	goto invalid_dwarf;

      /* Get attribute name and form.  */
      Dwarf_Attribute attr;
      const unsigned char *remembered_attrp = attrp;

      get_uleb128 (attr.code, attrp, endp);
      if (unlikely (attrp >= endp))
	goto invalid_dwarf;
      get_uleb128 (attr.form, attrp, endp);

      /* We can stop if we found the attribute with value zero.  */
      if (attr.code == 0 && attr.form == 0)
	/* Do not return 0 here - there would be no way to
	   distinguish this value from the attribute at offset 0.
	   Instead we return +1 which would never be a valid
	   offset of an attribute.  */
        return 1l;

      /* If we are not to OFFSET_ATTRP yet, we just have to skip
	 the values of the intervening attributes.  */
      if (remembered_attrp >= offset_attrp)
	{
	  /* Fill in the rest.  */
	  attr.valp = (unsigned char *) die_addr;
	  attr.cu = die->cu;

	  /* Now call the callback function.  */
	  if (callback (&attr, arg) != DWARF_CB_OK)
	    /* Return the offset of the start of the attribute, so that
	       dwarf_getattrs() can be restarted from this point if the
	       caller so desires.  */
	    return remembered_attrp - abbrevp->attrp;
	}

      /* Skip over the rest of this attribute (if there is any).  */
      if (attr.form != 0)
	{
	  size_t len = __libdw_form_val_len (die->cu, attr.form, die_addr);
	  if (unlikely (len == (size_t) -1l))
	    /* Something wrong with the file.  */
	    return -1l;

	  // __libdw_form_val_len will have done a bounds check.
	  die_addr += len;
	}
    }
  /* NOTREACHED */
}
