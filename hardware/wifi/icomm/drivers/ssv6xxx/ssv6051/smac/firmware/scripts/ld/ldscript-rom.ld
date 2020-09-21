/**
 * Link Script for SRAM booting for SSV6200.
 *
 * This link script assumes that the boot mode is configured to SARM mode, 
 * whcih means the booting is from SRAM. The image has been written to 
 * SRAM before enable CPU.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)



MEMORY
{
    SRAM(WX): ORIGIN = 0x00000000, LENGTH = 160
}


SECTIONS
{




}


