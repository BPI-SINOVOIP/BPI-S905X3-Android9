#source: pr20253-1.s
#as: --x32
#ld: -melf32_x86_64
#objdump: -dw
#notarget: x86_64-*-nacl*

.*: +file format .*


Disassembly of section .text:

0+40008c <foo>:
 +[a-f0-9]+:	c3                   	retq   

0+40008d <bar>:
 +[a-f0-9]+:	c3                   	retq   

0+40008e <_start>:
 +[a-f0-9]+:	ff 15 28 00 20 00    	callq  \*0x200028\(%rip\)        # 6000bc <_start\+0x20002e>
 +[a-f0-9]+:	ff 25 2a 00 20 00    	jmpq   \*0x20002a\(%rip\)        # 6000c4 <_start\+0x200036>
 +[a-f0-9]+:	48 c7 05 1f 00 20 00 00 00 00 00 	movq   \$0x0,0x20001f\(%rip\)        # 6000c4 <_start\+0x200036>
 +[a-f0-9]+:	48 83 3d 0f 00 20 00 00 	cmpq   \$0x0,0x20000f\(%rip\)        # 6000bc <_start\+0x20002e>
 +[a-f0-9]+:	48 3b 0d 08 00 20 00 	cmp    0x200008\(%rip\),%rcx        # 6000bc <_start\+0x20002e>
 +[a-f0-9]+:	48 3b 0d 09 00 20 00 	cmp    0x200009\(%rip\),%rcx        # 6000c4 <_start\+0x200036>
#pass
