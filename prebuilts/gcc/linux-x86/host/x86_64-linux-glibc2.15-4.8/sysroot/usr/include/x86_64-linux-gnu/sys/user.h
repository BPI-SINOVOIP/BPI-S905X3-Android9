/* Copyright (C) 2001, 2002, 2004, 2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _SYS_USER_H
#define _SYS_USER_H	1

/* The whole purpose of this file is for GDB and GDB only.  Don't read
   too much into it.  Don't use it for anything other than GDB unless
   you know what you are doing.  */

#include <bits/wordsize.h>
#include <unistd.h>

#if __WORDSIZE == 64

struct user_fpregs_struct
{
  unsigned short int	cwd;
  unsigned short int	swd;
  unsigned short int	ftw;
  unsigned short int	fop;
  unsigned long int	rip;
  unsigned long int	rdp;
  unsigned int		mxcsr;
  unsigned int		mxcr_mask;
  unsigned int		st_space[32];   /* 8*16 bytes for each FP-reg = 128 bytes */
  unsigned int		xmm_space[64];  /* 16*16 bytes for each XMM-reg = 256 bytes */
  unsigned int		padding[24];
};

struct user_regs_struct
{
  unsigned long int r15;
  unsigned long int r14;
  unsigned long int r13;
  unsigned long int r12;
  unsigned long int rbp;
  unsigned long int rbx;
  unsigned long int r11;
  unsigned long int r10;
  unsigned long int r9;
  unsigned long int r8;
  unsigned long int rax;
  unsigned long int rcx;
  unsigned long int rdx;
  unsigned long int rsi;
  unsigned long int rdi;
  unsigned long int orig_rax;
  unsigned long int rip;
  unsigned long int cs;
  unsigned long int eflags;
  unsigned long int rsp;
  unsigned long int ss;
  unsigned long int fs_base;
  unsigned long int gs_base;
  unsigned long int ds;
  unsigned long int es;
  unsigned long int fs;
  unsigned long int gs;
};

struct user
{
  struct user_regs_struct	regs;
  int				u_fpvalid;
  struct user_fpregs_struct	i387;
  unsigned long int		u_tsize;
  unsigned long int		u_dsize;
  unsigned long int		u_ssize;
  unsigned long int		start_code;
  unsigned long int		start_stack;
  long int			signal;
  int				reserved;
  struct user_regs_struct*	u_ar0;
  struct user_fpregs_struct*	u_fpstate;
  unsigned long int		magic;
  char				u_comm [32];
  unsigned long int		u_debugreg [8];
};

#else
/* These are the 32-bit x86 structures.  */
struct user_fpregs_struct
{
  long int cwd;
  long int swd;
  long int twd;
  long int fip;
  long int fcs;
  long int foo;
  long int fos;
  long int st_space [20];
};

struct user_fpxregs_struct
{
  unsigned short int cwd;
  unsigned short int swd;
  unsigned short int twd;
  unsigned short int fop;
  long int fip;
  long int fcs;
  long int foo;
  long int fos;
  long int mxcsr;
  long int reserved;
  long int st_space[32];   /* 8*16 bytes for each FP-reg = 128 bytes */
  long int xmm_space[32];  /* 8*16 bytes for each XMM-reg = 128 bytes */
  long int padding[56];
};

struct user_regs_struct
{
  long int ebx;
  long int ecx;
  long int edx;
  long int esi;
  long int edi;
  long int ebp;
  long int eax;
  long int xds;
  long int xes;
  long int xfs;
  long int xgs;
  long int orig_eax;
  long int eip;
  long int xcs;
  long int eflags;
  long int esp;
  long int xss;
};

struct user
{
  struct user_regs_struct	regs;
  int				u_fpvalid;
  struct user_fpregs_struct	i387;
  unsigned long int		u_tsize;
  unsigned long int		u_dsize;
  unsigned long int		u_ssize;
  unsigned long int		start_code;
  unsigned long int		start_stack;
  long int			signal;
  int				reserved;
  struct user_regs_struct*	u_ar0;
  struct user_fpregs_struct*	u_fpstate;
  unsigned long int		magic;
  char				u_comm [32];
  int				u_debugreg [8];
};
#endif  /* __WORDSIZE */

#define PAGE_SIZE		(sysconf(_SC_PAGESIZE))
#define PAGE_MASK		(~(PAGE_SIZE-1))
#define NBPG			PAGE_SIZE
#define UPAGES			1
#define HOST_TEXT_START_ADDR	(u.start_code)
#define HOST_STACK_END_ADDR	(u.start_stack + u.u_ssize * NBPG)

#endif	/* _SYS_USER_H */
