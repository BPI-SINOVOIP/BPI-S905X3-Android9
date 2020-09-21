/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdint.h>
#include <gpxe/pci.h>
#include <realmode.h>

/** @file
 *
 * PCI configuration space access via PCI BIOS
 *
 */

/**
 * Determine maximum PCI bus number within system
 *
 * @ret max_bus		Maximum bus number
 */
static int pcibios_max_bus ( void ) {
	int discard_a, discard_D;
	uint8_t max_bus;

	__asm__ __volatile__ ( REAL_CODE ( "stc\n\t"
					   "int $0x1a\n\t"
					   "jnc 1f\n\t"
					   "xorw %%cx, %%cx\n\t"
					   "\n1:\n\t" )
			       : "=c" ( max_bus ), "=a" ( discard_a ),
				 "=D" ( discard_D )
			       : "a" ( PCIBIOS_INSTALLATION_CHECK >> 16 ),
				 "D" ( 0 )
			       : "ebx", "edx" );

	return max_bus;
}

/**
 * Read configuration space via PCI BIOS
 *
 * @v pci	PCI device
 * @v command	PCI BIOS command
 * @v value	Value read
 * @ret rc	Return status code
 */
int pcibios_read ( struct pci_device *pci, uint32_t command, uint32_t *value ){
	int discard_b, discard_D;
	int status;

	__asm__ __volatile__ ( REAL_CODE ( "stc\n\t"
					   "int $0x1a\n\t"
					   "jnc 1f\n\t"
					   "xorl %%eax, %%eax\n\t"
					   "decl %%eax\n\t"
					   "movl %%eax, %%ecx\n\t"
					   "\n1:\n\t" )
			       : "=a" ( status ), "=b" ( discard_b ),
				 "=c" ( *value ), "=D" ( discard_D )
			       : "a" ( command >> 16 ), "D" ( command ),
			         "b" ( PCI_BUSDEVFN ( pci->bus, pci->devfn ) )
			       : "edx" );

	return ( ( status >> 8 ) & 0xff );
}

/**
 * Write configuration space via PCI BIOS
 *
 * @v pci	PCI device
 * @v command	PCI BIOS command
 * @v value	Value to be written
 * @ret rc	Return status code
 */
int pcibios_write ( struct pci_device *pci, uint32_t command, uint32_t value ){
	int discard_b, discard_c, discard_D;
	int status;

	__asm__ __volatile__ ( REAL_CODE ( "stc\n\t"
					   "int $0x1a\n\t"
					   "jnc 1f\n\t"
					   "movb $0xff, %%ah\n\t"
					   "\n1:\n\t" )
			       : "=a" ( status ), "=b" ( discard_b ),
				 "=c" ( discard_c ), "=D" ( discard_D )
			       : "a" ( command >> 16 ),	"D" ( command ),
			         "b" ( PCI_BUSDEVFN ( pci->bus, pci->devfn ) ),
				 "c" ( value )
			       : "edx" );
	
	return ( ( status >> 8 ) & 0xff );
}

PROVIDE_PCIAPI ( pcbios, pci_max_bus, pcibios_max_bus );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_read_config_byte );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_read_config_word );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_read_config_dword );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_write_config_byte );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_write_config_word );
PROVIDE_PCIAPI_INLINE ( pcbios, pci_write_config_dword );
