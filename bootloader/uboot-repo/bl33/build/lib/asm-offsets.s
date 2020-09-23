	.arch armv8-a+fp+simd
	.file	"asm-offsets.c"
// GNU C (crosstool-NG linaro-1.13.1-4.8-2013.11 - Linaro GCC 2013.10) version 4.8.3 20131111 (prerelease) (aarch64-none-elf)
//	compiled by GNU C version 4.1.3 20080704 (prerelease) (Ubuntu 4.1.2-27ubuntu1), GMP version 5.0.2, MPFR version 3.1.0, MPC version 0.9
// GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
// options passed:  -nostdinc -I include -I ../include
// -I ../arch/arm/include -I ../. -I .
// -iprefix /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/
// -D __KERNEL__ -D __UBOOT__ -D CONFIG_SYS_TEXT_BASE=0x01000000 -D __ARM__
// -D DO_DEPS_ONLY -D KBUILD_STR(s)=#s
// -D KBUILD_BASENAME=KBUILD_STR(asm_offsets)
// -D KBUILD_MODNAME=KBUILD_STR(asm_offsets)
// -isystem /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/include
// -include ../include/linux/kconfig.h -MD lib/.asm-offsets.s.d
// ../lib/asm-offsets.c -march=armv8-a -mstrict-align
// -auxbase-strip lib/asm-offsets.s -g -Os -Wall -Wstrict-prototypes
// -Wno-format-security -Wno-format-nonliteral -Werror -fno-builtin
// -ffreestanding -fno-stack-protector -fstack-usage -ffunction-sections
// -fdata-sections -ffixed-r9 -fno-common -ffixed-x18 -fverbose-asm
// options enabled:  -faggressive-loop-optimizations -fauto-inc-dec
// -fbranch-count-reg -fcaller-saves -fcombine-stack-adjustments
// -fcompare-elim -fcprop-registers -fcrossjumping -fcse-follow-jumps
// -fdata-sections -fdefer-pop -fdelete-null-pointer-checks -fdevirtualize
// -fdwarf2-cfi-asm -fearly-inlining -feliminate-unused-debug-types
// -fexpensive-optimizations -fforward-propagate -ffunction-cse
// -ffunction-sections -fgcse -fgcse-lm -fgnu-runtime
// -fguess-branch-probability -fhoist-adjacent-loads -fident
// -fif-conversion -fif-conversion2 -findirect-inlining -finline
// -finline-atomics -finline-functions -finline-functions-called-once
// -finline-small-functions -fipa-cp -fipa-profile -fipa-pure-const
// -fipa-reference -fipa-sra -fira-hoist-pressure -fira-share-save-slots
// -fira-share-spill-slots -fivopts -fkeep-static-consts
// -fleading-underscore -fmath-errno -fmerge-constants
// -fmerge-debug-strings -fmove-loop-invariants -fomit-frame-pointer
// -foptimize-register-move -foptimize-sibling-calls -fpartial-inlining
// -fpeephole -fpeephole2 -fprefetch-loop-arrays -free -freg-struct-return
// -fregmove -freorder-blocks -freorder-functions -frerun-cse-after-loop
// -fsched-critical-path-heuristic -fsched-dep-count-heuristic
// -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
// -fsched-rank-heuristic -fsched-spec -fsched-spec-insn-heuristic
// -fsched-stalled-insns-dep -fschedule-insns2 -fsection-anchors
// -fshow-column -fshrink-wrap -fsigned-zeros -fsplit-ivs-in-unroller
// -fsplit-wide-types -fstrict-aliasing -fstrict-overflow
// -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
// -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp
// -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-coalesce-vars
// -ftree-copy-prop -ftree-copyrename -ftree-cselim -ftree-dce
// -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre
// -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
// -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
// -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink
// -ftree-slp-vectorize -ftree-slsr -ftree-sra -ftree-switch-conversion
// -ftree-tail-merge -ftree-ter -ftree-vect-loop-version -ftree-vrp
// -funit-at-a-time -fvar-tracking -fvar-tracking-assignments -fverbose-asm
// -fzero-initialized-in-bss -mlittle-endian -momit-leaf-frame-pointer
// -mstrict-align

	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.startup.main,"ax",%progbits
	.align	2
	.global	main
	.type	main, %function
main:
.LFB184:
	.file 1 "../lib/asm-offsets.c"
	.loc 1 20 0
	.cfi_startproc
	.loc 1 22 0
	// Start of user assembly
// 22 "../lib/asm-offsets.c" 1
	
.ascii "->GENERATED_GBL_DATA_SIZE 320 (sizeof(struct global_data) + 15) & ~15"	//
// 0 "" 2
	.loc 1 25 0
// 25 "../lib/asm-offsets.c" 1
	
.ascii "->GENERATED_BD_INFO_SIZE 160 (sizeof(struct bd_info) + 15) & ~15"	//
// 0 "" 2
	.loc 1 28 0
// 28 "../lib/asm-offsets.c" 1
	
.ascii "->GD_SIZE 320 sizeof(struct global_data)"	//
// 0 "" 2
	.loc 1 30 0
// 30 "../lib/asm-offsets.c" 1
	
.ascii "->GD_BD 0 offsetof(struct global_data, bd)"	//
// 0 "" 2
	.loc 1 37 0
// 37 "../lib/asm-offsets.c" 1
	
.ascii "->GD_RELOCADDR 88 offsetof(struct global_data, relocaddr)"	//
// 0 "" 2
	.loc 1 39 0
// 39 "../lib/asm-offsets.c" 1
	
.ascii "->GD_RELOC_OFF 128 offsetof(struct global_data, reloc_off)"	//
// 0 "" 2
	.loc 1 41 0
// 41 "../lib/asm-offsets.c" 1
	
.ascii "->GD_START_ADDR_SP 120 offsetof(struct global_data, start_addr_sp)"	//
// 0 "" 2
	.loc 1 46 0
	// End of user assembly
	mov	w0, 0	//,
	ret
	.cfi_endproc
.LFE184:
	.size	main, .-main
	.text
.Letext0:
	.file 2 "../arch/arm/include/asm/types.h"
	.file 3 "../include/linux/types.h"
	.file 4 "../include/asm-generic/u-boot.h"
	.file 5 "../include/net.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x341
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF55
	.byte	0x1
	.4byte	.LASF56
	.4byte	.LASF57
	.4byte	.Ldebug_ranges0+0
	.8byte	0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF3
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF4
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF7
	.uleb128 0x4
	.4byte	.LASF11
	.byte	0x2
	.byte	0x35
	.4byte	0x30
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF9
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF10
	.uleb128 0x4
	.4byte	.LASF12
	.byte	0x3
	.byte	0x59
	.4byte	0x30
	.uleb128 0x5
	.byte	0x8
	.uleb128 0x6
	.4byte	0x29
	.4byte	0xa5
	.uleb128 0x7
	.4byte	0x73
	.byte	0x5
	.byte	0
	.uleb128 0x8
	.byte	0x10
	.byte	0x4
	.byte	0x7b
	.4byte	0xc6
	.uleb128 0x9
	.4byte	.LASF13
	.byte	0x4
	.byte	0x7c
	.4byte	0x88
	.byte	0
	.uleb128 0x9
	.4byte	.LASF14
	.byte	0x4
	.byte	0x7d
	.4byte	0x88
	.byte	0x8
	.byte	0
	.uleb128 0xa
	.4byte	.LASF35
	.byte	0x98
	.byte	0x4
	.byte	0x1b
	.4byte	0x1b7
	.uleb128 0x9
	.4byte	.LASF15
	.byte	0x4
	.byte	0x1c
	.4byte	0x30
	.byte	0
	.uleb128 0x9
	.4byte	.LASF16
	.byte	0x4
	.byte	0x1d
	.4byte	0x68
	.byte	0x8
	.uleb128 0x9
	.4byte	.LASF17
	.byte	0x4
	.byte	0x1e
	.4byte	0x30
	.byte	0x10
	.uleb128 0x9
	.4byte	.LASF18
	.byte	0x4
	.byte	0x1f
	.4byte	0x30
	.byte	0x18
	.uleb128 0x9
	.4byte	.LASF19
	.byte	0x4
	.byte	0x20
	.4byte	0x30
	.byte	0x20
	.uleb128 0x9
	.4byte	.LASF20
	.byte	0x4
	.byte	0x21
	.4byte	0x30
	.byte	0x28
	.uleb128 0x9
	.4byte	.LASF21
	.byte	0x4
	.byte	0x22
	.4byte	0x30
	.byte	0x30
	.uleb128 0x9
	.4byte	.LASF22
	.byte	0x4
	.byte	0x24
	.4byte	0x30
	.byte	0x38
	.uleb128 0x9
	.4byte	.LASF23
	.byte	0x4
	.byte	0x25
	.4byte	0x30
	.byte	0x40
	.uleb128 0x9
	.4byte	.LASF24
	.byte	0x4
	.byte	0x26
	.4byte	0x30
	.byte	0x48
	.uleb128 0x9
	.4byte	.LASF25
	.byte	0x4
	.byte	0x32
	.4byte	0x30
	.byte	0x50
	.uleb128 0x9
	.4byte	.LASF26
	.byte	0x4
	.byte	0x33
	.4byte	0x30
	.byte	0x58
	.uleb128 0x9
	.4byte	.LASF27
	.byte	0x4
	.byte	0x34
	.4byte	0x95
	.byte	0x60
	.uleb128 0x9
	.4byte	.LASF28
	.byte	0x4
	.byte	0x35
	.4byte	0x37
	.byte	0x66
	.uleb128 0x9
	.4byte	.LASF29
	.byte	0x4
	.byte	0x36
	.4byte	0x30
	.byte	0x68
	.uleb128 0x9
	.4byte	.LASF30
	.byte	0x4
	.byte	0x37
	.4byte	0x30
	.byte	0x70
	.uleb128 0x9
	.4byte	.LASF31
	.byte	0x4
	.byte	0x78
	.4byte	0x88
	.byte	0x78
	.uleb128 0x9
	.4byte	.LASF32
	.byte	0x4
	.byte	0x79
	.4byte	0x88
	.byte	0x80
	.uleb128 0x9
	.4byte	.LASF33
	.byte	0x4
	.byte	0x7e
	.4byte	0x1b7
	.byte	0x88
	.byte	0
	.uleb128 0x6
	.4byte	0xa5
	.4byte	0x1c7
	.uleb128 0x7
	.4byte	0x73
	.byte	0
	.byte	0
	.uleb128 0x4
	.4byte	.LASF34
	.byte	0x4
	.byte	0x80
	.4byte	0xc6
	.uleb128 0xb
	.byte	0x8
	.4byte	0x1c7
	.uleb128 0xa
	.4byte	.LASF36
	.byte	0x60
	.byte	0x5
	.byte	0x51
	.4byte	0x275
	.uleb128 0x9
	.4byte	.LASF37
	.byte	0x5
	.byte	0x52
	.4byte	0x275
	.byte	0
	.uleb128 0x9
	.4byte	.LASF38
	.byte	0x5
	.byte	0x53
	.4byte	0x95
	.byte	0x10
	.uleb128 0x9
	.4byte	.LASF39
	.byte	0x5
	.byte	0x54
	.4byte	0x4c
	.byte	0x18
	.uleb128 0x9
	.4byte	.LASF40
	.byte	0x5
	.byte	0x55
	.4byte	0x4c
	.byte	0x1c
	.uleb128 0x9
	.4byte	.LASF41
	.byte	0x5
	.byte	0x57
	.4byte	0x29f
	.byte	0x20
	.uleb128 0x9
	.4byte	.LASF42
	.byte	0x5
	.byte	0x58
	.4byte	0x2be
	.byte	0x28
	.uleb128 0x9
	.4byte	.LASF43
	.byte	0x5
	.byte	0x59
	.4byte	0x2d3
	.byte	0x30
	.uleb128 0x9
	.4byte	.LASF44
	.byte	0x5
	.byte	0x5a
	.4byte	0x2e4
	.byte	0x38
	.uleb128 0x9
	.4byte	.LASF45
	.byte	0x5
	.byte	0x5e
	.4byte	0x2d3
	.byte	0x40
	.uleb128 0x9
	.4byte	.LASF46
	.byte	0x5
	.byte	0x5f
	.4byte	0x299
	.byte	0x48
	.uleb128 0x9
	.4byte	.LASF47
	.byte	0x5
	.byte	0x60
	.4byte	0x4c
	.byte	0x50
	.uleb128 0x9
	.4byte	.LASF48
	.byte	0x5
	.byte	0x61
	.4byte	0x93
	.byte	0x58
	.byte	0
	.uleb128 0x6
	.4byte	0x81
	.4byte	0x285
	.uleb128 0x7
	.4byte	0x73
	.byte	0xf
	.byte	0
	.uleb128 0xc
	.4byte	0x4c
	.4byte	0x299
	.uleb128 0xd
	.4byte	0x299
	.uleb128 0xd
	.4byte	0x1d2
	.byte	0
	.uleb128 0xb
	.byte	0x8
	.4byte	0x1d8
	.uleb128 0xb
	.byte	0x8
	.4byte	0x285
	.uleb128 0xc
	.4byte	0x4c
	.4byte	0x2be
	.uleb128 0xd
	.4byte	0x299
	.uleb128 0xd
	.4byte	0x93
	.uleb128 0xd
	.4byte	0x4c
	.byte	0
	.uleb128 0xb
	.byte	0x8
	.4byte	0x2a5
	.uleb128 0xc
	.4byte	0x4c
	.4byte	0x2d3
	.uleb128 0xd
	.4byte	0x299
	.byte	0
	.uleb128 0xb
	.byte	0x8
	.4byte	0x2c4
	.uleb128 0xe
	.4byte	0x2e4
	.uleb128 0xd
	.4byte	0x299
	.byte	0
	.uleb128 0xb
	.byte	0x8
	.4byte	0x2d9
	.uleb128 0xf
	.4byte	.LASF58
	.byte	0x4
	.byte	0x5
	.2byte	0x1f3
	.4byte	0x310
	.uleb128 0x10
	.4byte	.LASF49
	.sleb128 0
	.uleb128 0x10
	.4byte	.LASF50
	.sleb128 1
	.uleb128 0x10
	.4byte	.LASF51
	.sleb128 2
	.uleb128 0x10
	.4byte	.LASF52
	.sleb128 3
	.byte	0
	.uleb128 0x11
	.4byte	.LASF59
	.byte	0x1
	.byte	0x13
	.4byte	0x4c
	.8byte	.LFB184
	.8byte	.LFE184-.LFB184
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x12
	.4byte	.LASF53
	.byte	0x5
	.byte	0x6b
	.4byte	0x299
	.uleb128 0x13
	.4byte	.LASF54
	.byte	0x5
	.2byte	0x1f9
	.4byte	0x2ea
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x15
	.byte	0x1
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x15
	.byte	0x1
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.LFB184
	.8byte	.LFE184-.LFB184
	.8byte	0
	.8byte	0
	.section	.debug_ranges,"",%progbits
.Ldebug_ranges0:
	.8byte	.LFB184
	.8byte	.LFE184
	.8byte	0
	.8byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF53:
	.string	"eth_current"
.LASF22:
	.string	"bi_arm_freq"
.LASF27:
	.string	"bi_enetaddr"
.LASF44:
	.string	"halt"
.LASF40:
	.string	"state"
.LASF48:
	.string	"priv"
.LASF31:
	.string	"bi_arch_number"
.LASF19:
	.string	"bi_flashoffset"
.LASF9:
	.string	"long int"
.LASF55:
	.string	"GNU C 4.8.3 20131111 (prerelease) -march=armv8-a -mstrict-align -g -Os -fno-builtin -ffreestanding -fno-stack-protector -fstack-usage -ffunction-sections -fdata-sections -ffixed-r9 -fno-common -ffixed-x18"
.LASF41:
	.string	"init"
.LASF25:
	.string	"bi_bootflags"
.LASF29:
	.string	"bi_intfreq"
.LASF54:
	.string	"net_state"
.LASF28:
	.string	"bi_ethspeed"
.LASF26:
	.string	"bi_ip_addr"
.LASF38:
	.string	"enetaddr"
.LASF39:
	.string	"iobase"
.LASF52:
	.string	"NETLOOP_FAIL"
.LASF13:
	.string	"start"
.LASF2:
	.string	"short unsigned int"
.LASF18:
	.string	"bi_flashsize"
.LASF24:
	.string	"bi_ddr_freq"
.LASF23:
	.string	"bi_dsp_freq"
.LASF43:
	.string	"recv"
.LASF15:
	.string	"bi_memstart"
.LASF0:
	.string	"unsigned char"
.LASF1:
	.string	"long unsigned int"
.LASF59:
	.string	"main"
.LASF20:
	.string	"bi_sramstart"
.LASF33:
	.string	"bi_dram"
.LASF12:
	.string	"ulong"
.LASF5:
	.string	"unsigned int"
.LASF42:
	.string	"send"
.LASF35:
	.string	"bd_info"
.LASF7:
	.string	"long long unsigned int"
.LASF16:
	.string	"bi_memsize"
.LASF56:
	.string	"../lib/asm-offsets.c"
.LASF11:
	.string	"phys_size_t"
.LASF36:
	.string	"eth_device"
.LASF8:
	.string	"sizetype"
.LASF6:
	.string	"long long int"
.LASF50:
	.string	"NETLOOP_RESTART"
.LASF51:
	.string	"NETLOOP_SUCCESS"
.LASF47:
	.string	"index"
.LASF58:
	.string	"net_loop_state"
.LASF4:
	.string	"short int"
.LASF21:
	.string	"bi_sramsize"
.LASF30:
	.string	"bi_busfreq"
.LASF49:
	.string	"NETLOOP_CONTINUE"
.LASF57:
	.string	"/media/dangku/mywork/m5/android/BPI-S905X3-Android9/bootloader/uboot-repo/bl33/build"
.LASF17:
	.string	"bi_flashstart"
.LASF10:
	.string	"char"
.LASF32:
	.string	"bi_boot_params"
.LASF3:
	.string	"signed char"
.LASF45:
	.string	"write_hwaddr"
.LASF46:
	.string	"next"
.LASF14:
	.string	"size"
.LASF34:
	.string	"bd_t"
.LASF37:
	.string	"name"
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.8-2013.11 - Linaro GCC 2013.10) 4.8.3 20131111 (prerelease)"
