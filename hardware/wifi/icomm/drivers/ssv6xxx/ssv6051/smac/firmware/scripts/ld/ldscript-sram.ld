/**
 * Link Script for Cabrio SRAM Booting
 * BOOT-TYPE: MODE_RABOOT_RARUN
 *
 * This link script is used for SRAM booting. All image sections
 * (.TEXT .DATA .BSS ...etc) are assumed to be on the SRAM and 
 * running on SRAM. Note that the image shall be moved to SRAM 
 * before running.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)



MEMORY
{
    SRAM(RWX): ORIGIN = 0x00000000, LENGTH = 160K
}


SECTIONS
{
    .STARTUP :
    {
	*(.startup)

    } >SRAM

    .TEXT :
    {
	. = ALIGN(4);
	*(.text)
	*(.text*)
	*(.rodata)
	*(.rodata*)
	*(.glue_7)
	*(.glue_7t)
	. = ALIGN(4);
	_etext = .;
	__text_end = _etext;

    } >SRAM
    __text_end = .;

    .DATA :
    {
	. = ALIGN(4);
	*(.data)
	*(.data*)
	. = ALIGN(4);
	_edat = .;

    } >SRAM

    .BSS :
    {
	. = ALIGN(4);
 	_sbss = .;
	__bss_beg = _sbss;
	*(.bss)
	*(.bss*)
 	*(COMMON)
	. = ALIGN(4);
	_ebss = .;
	__bss_end = _ebss;

    } >SRAM

    /* Assign Heap & Stack VMA. These variables are used
       in C code. */
    . = ALIGN(4);
    __heap_beg = .;
    __stack_beg = 0 + 160K - 4;


    /* This section is only for debugging purpose and shall
       be removed. */
    .DMSG :
    {
	__dmsg_beg = .;
	*(.dbgmsg)
	__dmsg_end = .;

    } >SRAM

}


