/**
 * Link Script for Cabrio Flash-Booting/Flash-Running
 * BOOT-TYPE: MODE_FLBOOT_FLRUN
 *
 * This link script is created only for Flash-Booting/Flash-Running mode.
 * The .TEXT is located and executed on flash. .DATA and .BSS shall be 
 * copied to corresponding SRAM region before running C on flash.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)


MEMORY
{
    SRAM(RWX): ORIGIN = 0x02000000, LENGTH = 160K
    FLASH(RX): ORIGIN = 0x00000000, LENGTH = 16M
}


SECTIONS
{

    .STARTUP :
    {
	*(.startup)

    } >FLASH

    .TEXT :
    {
	. = ALIGN(4);
	*(.text)
	*(.text*)
/*
	*(.rodata)
	*(.rodata*)
*/
	*(.glue_7)
	*(.glue_7t)
	. = ALIGN(4);
	_etext = .;
	__text_end = _etext;

    } >FLASH

    /* The LMA shall be kept and VMA shall be set to SRAM
       region. */
    __DATA_LMA = .;
    .DATA : AT(__DATA_LMA)
    {
        . = ALIGN(4);
	_sdat = .;
	__dat_beg = _sdat;
        *(.rodata)
        *(.rodata*)
        *(.data)
        *(.data*)
	. = ALIGN(4);
        _edat = .;
	__dat_end = _edat;

    } >SRAM

    /* We can ignore AT(_BSS_LMA) because the LMA has changed
       at .DATA */
    __BSS_LMA = __DATA_LMA + SIZEOF(.DATA);
    .BSS : AT(__BSS_LMA)
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
    __stack_beg = ORIGIN(SRAM) + LENGTH(SRAM) - 4; 


    /* This section is only for debugging purpose and shall
       be removed. */
    .DMSG :
    {
        __dmsg_beg = .;
        *(.dbgmsg)
        __dmsg_end = .;

    } >SRAM

}


