/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <gpxe/uaccess.h>
#include <gpxe/efi/efi.h>

/** @file
 *
 * gPXE user access API for EFI
 *
 */

PROVIDE_UACCESS_INLINE ( efi, phys_to_user );
PROVIDE_UACCESS_INLINE ( efi, user_to_phys );
PROVIDE_UACCESS_INLINE ( efi, virt_to_user );
PROVIDE_UACCESS_INLINE ( efi, user_to_virt );
PROVIDE_UACCESS_INLINE ( efi, userptr_add );
PROVIDE_UACCESS_INLINE ( efi, memcpy_user );
PROVIDE_UACCESS_INLINE ( efi, memmove_user );
PROVIDE_UACCESS_INLINE ( efi, memset_user );
PROVIDE_UACCESS_INLINE ( efi, strlen_user );
PROVIDE_UACCESS_INLINE ( efi, memchr_user );
