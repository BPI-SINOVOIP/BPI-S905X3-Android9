/**
 * Link Script for Cabrio Flash-Booting/SRAM-Running
 * BOOT-TYPE: MODE_FLBOOT_RARUN
 *
 * This link script is created only for Flash-Booting/SRAM-Running mode.
 * Booting is from flash. Image is executed after boot code(running on flash)
 * loads image (.TEXT .DATA .BSS ...etc) from flash to SRAM region. 
 * The system shall provide remapping function.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)


MEMORY
{
    SRAM_PA(RWX): ORIGIN = 0x02000000, LENGTH = 160K
    SRAM_VA(RWX): ORIGIN = 0x00000000, LENGTH = 160K
}


SECTIONS
{
    .STARTUP :
    {
        *(.startup)

    } >SRAM_VA

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

    } >SRAM_VA

    .DATA :
    {
        . = ALIGN(4);
        _sdat = .;
        __dat_beg = _sdat;
        *(.data)
        *(.data*)
	. = ALIGN(4);
        _edat = .;
        __dat_end = _edat;

    } >SRAM_VA

    /* We can ignore AT(_BSS_LMA) because the LMA has changed
       at .DATA */
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

    } >SRAM_VA

    /* Assign Heap & Stack VMA. These variables are used
       in C code. */
    . = ALIGN(4);
    __heap_beg = .;
    __stack_beg = ORIGIN(SRAM_VA) + LENGTH(SRAM_VA) - 4;
    __sram_pa = ORIGIN(SRAM_PA);
    __image_len = __dat_end;

    /* This section is only for debugging purpose and shall
       be removed. */
    .DMSG :
    {
        __dmsg_beg = .;
        *(.dbgmsg)
        __dmsg_end = .;

    } >SRAM_VA

}
