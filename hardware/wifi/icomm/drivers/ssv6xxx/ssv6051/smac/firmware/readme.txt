
SSV6200(Cabrio) Project
========================

0. Source tree structure

   wifi-soc
      |
      +- doc
      |
      +- ssv6200
           |
           +- host
           |
           +- soc
               |
               + apps
               |
               + bsp
               |
               + cli
               |
               + deconfig
               | 
               + driver
               |
               + image
               |
               + include
               |
               + lib
               | 
               + rtos
               |
               + scripts
               |
               + utility
               |
               + Makefile
               |
               + config.mk
               |
               + rules.mk
               |
               + readme.txt
             
           

1. How to make project ?
   
   wifi-soc/ssv6200/soc$ make config
   wifi-soc/ssv6200/soc$ make

2. Where is the image file ?
   
   wifi-soc/ssv6200/soc/image$ ls -al
   ssv6200-uart.asm  ssv6200-uart.bin  ssv6200-uart.elf  ssv6200-uart.map
	 		    ^
                       <Image file>





