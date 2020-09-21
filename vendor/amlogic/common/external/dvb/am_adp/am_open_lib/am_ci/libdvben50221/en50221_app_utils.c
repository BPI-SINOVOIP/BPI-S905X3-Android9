#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
    en50221 encoder An implementation for libdvb
    an implementation for the en50221 transport layer

    Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
    Copyright (C) 2005 Julian Scheel (julian at jusst dot de)
    Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include "en50221_app_utils.h"

struct en50221_app_public_resource_id
	*en50221_app_decode_public_resource_id(struct en50221_app_public_resource_id *idf,
					       uint32_t resource_id)
{
	// reject private resources
	if ((resource_id & 0xc0000000) == 0xc0000000)
		return NULL;

	idf->resource_class = (resource_id >> 16) & 0xffff;	// use the resource_id as the MSBs of class
	idf->resource_type = (resource_id >> 6) & 0x3ff;
	idf->resource_version = resource_id & 0x3f;
	return idf;
}
