#include <config.h>
#include <stdarg.h>

#if (CONFIG_SIM_PLATFORM == 0)
#include <uart/drv_uart.h>
#endif

#include "lib-impl.h"
#include <log.h>

#ifdef USE_SERIAL_DRV
#include <bsp/serial.h>
#endif // USE_SERIAL_DRV


#if (CONFIG_SIM_PLATFORM == 0)



#ifdef __REG_PRINT__
#define REG32(addr)              (*(volatile u32 *)(addr))
#endif


static s32 printchar(s8 **str, const s8 * const out_end, s32 c)
{
    if (str) {
        if ((out_end != NULL) && ((size_t)*str > (size_t)out_end)) {
            printf("%s: Print buffer overflow.\n", __FUNCTION__);
            return (-1);
        } else {
            **str = (char)c;
            ++(*str);
        }
    } else {
#ifdef USE_SERIAL_DRV
        xSerialPutChar(CONSOLE_SERIAL_PORT, (signed portCHAR)c, portMAX_DELAY);
#else
        if (c=='\n') {
#ifndef __REG_PRINT__
            drv_uart_tx(SSV6XXX_UART0, 0x0a);
            drv_uart_tx(SSV6XXX_UART0, 0x0d);
#else//__REG_PRINT__
            REG32(0xc3001004) = 2;
            REG32(0xc3001008) = 0x0a;
            REG32(0xc3001008) = 0x0d;
#endif//__REG_PRINT__			

        }
        else
        {
#ifndef __REG_PRINT__        
            drv_uart_tx(SSV6XXX_UART0, c);
#else//__REG_PRINT__			
            REG32(0xc3001004) = 1;
            REG32(0xc3001008) = c;
#endif//__REG_PRINT__			
		}   
#endif // USE_SERIAL_DRV
    }
    return 0;
}


#define PAD_RIGHT 1
#define PAD_ZERO 2

#ifdef USE_SERIAL_DRV
#define MAX_PAD_WIDTH		(32)
static s32 __buf_prints (s8 **out, const s8 * const out_end, const s8 *string, s32 width)
{
    if ((s32)(out_end - *out - 1) < width) {
        printf("%s: Print buffer overflow.\n", __FUNCTION__);
        return 0;
    }

    memcpy(*out, string, width);
    *out += width;
	**out = 0;
	return width;
}

static s32 __ser_prints (s8 **out, const s8 * const out_end, const s8 *string, s32 width)
{
	(void)out; 
	(void)out_end;
	vSerialPutString(CONSOLE_SERIAL_PORT, (const signed char *)string, width);
	return width;
}

static s32 prints(s8 **out, const s8 * const out_end, const s8 *string, s32 width, s32 pad)
{
    s32 pc = 0;
    s32 len = strlen(string);
    signed char pad_buf[MAX_PAD_WIDTH];
    s32 (*__prints) (s8 **, const s8 * const out_end, const s8 *, s32) = out ? __buf_prints : __ser_prints;

    if (width > 0) {
        if (len >= width) 
            width = 0;
        else {
            width -= len;
            if (width > MAX_PAD_WIDTH)
                width = MAX_PAD_WIDTH;
            memset(pad_buf, ((pad & PAD_ZERO) ? '0' : ' '), width);
        }
    }

    if (!(pad & PAD_RIGHT) && (width > 0)) {
        __prints(out, out_end, (const s8 *)pad_buf, width);
        pc += width;
        width = 0;
    }

    __prints(out, out_end, string, len);
    pc += len;

    if (width > 0) {
        __prints(out, out_end, (const s8 *)pad_buf, width);
        pc += width;
    }

    return pc;
}


s32 printstr ( s8 **out, const s8 * const out_end, const char *s, size_t size)
{
    s32 (*__prints) (s8 **, const s8 const *, const char *s, s32) = out ? __buf_prints : __ser_prints;
    return __prints(out, out_end, s, size);
}


#else // USE_SERIAL_DRV


static s32 prints(s8 **out, const s8 * const out_end, const s8 *string, s32 width, s32 pad)
{
    register s32 pc = 0, padchar = ' ';

    if (width > 0) {
        register s32 len = 0;
        register const s8 *ptr;
        for (ptr = string; *ptr; ++ptr) ++len;
        if (len >= width) width = 0;
        else width -= len;
        if (pad & PAD_ZERO) padchar = '0';
    }
    if (!(pad & PAD_RIGHT)) {
        for ( ; width > 0; --width) {
            printchar (out, out_end, padchar);
            ++pc;
        }
    }
    for ( ; *string ; ++string) {
        printchar (out, out_end, *string);
        ++pc;
    }
    for ( ; width > 0; --width) {
        printchar (out, out_end, padchar);
        ++pc;
    }

    return pc;
}


s32 printstr ( s8 **out, const s8 * const out_end, const char *s, size_t size)
{
    size_t prn_size = size;
    while (size--)
        if ((printchar(out, out_end, s++))
            break;
    return (prn_size - size);
}
#endif // USE_SERIAL_DRV

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static s32 printi(s8 **out, const s8 * const out_end, s32 i, u32 b, s32 sg, s32 width, s32 pad, s32 letbase)
{
    s8 print_buf[PRINT_BUF_LEN];
    register s8 *s;
    register s32 t, neg = 0, pc = 0;
    register u32 u = (u32)i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints (out, out_end, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = (unsigned int)-i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    *s = '\0';

    while (u) {
        t = (unsigned int)u % b;
        if( t >= 10 )
            t += letbase - ('0' + 10);
        *--s = (char)(t + '0');
        u /= b;
    }

    if (neg) {
        if( width && (pad & PAD_ZERO) ) {
            printchar (out, out_end, (int)'-');
            ++pc;
            --width;
        }
        else {
            *--s = '-';
        }
    }
    return pc + prints (out, out_end, s, width, pad);
}

#define CHECK_PRINT_RET

s32 print( s8 **out, const s8 * const out_end, const s8 *_format, va_list args )
{
    //lint --e{613}
    #ifdef CHECK_PRINT_RET
	const s8 * const out_start = out ? *out : NULL;
    #endif // CHECK_PRINT_RET
    register s32 width, pad;
    register s32 pc = 0;
    s8 scr[2];
    const s8 *format;
    /*lint -save -e850 */
    for (format = _format; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            width = pad = 0;
            if (*format == '\0') break;
            if (*format == '%') goto out;
            if (*format == '-') {
                ++format;
                pad = PAD_RIGHT;
            }
            while (*format == '0') {
                ++format;
                pad |= PAD_ZERO;
            }
            while (*format >= '0' && *format <= '9') {
                width *= 10;
                width += *format - '0';
                ++format;
            }
            /*lint -save -e124 */
            if( *format == 's' ) { 
                register const char *s = (const char *)va_arg( args, long );
                pc += prints (out, out_end, s?s:"(null)", width, pad);
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("sxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }
            if( *format == 'd' ) {
                pc += printi (out, out_end, va_arg( args, int ), 10, 1, width, pad, 'a');
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("dxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }
            if( *format == 'x' ) {
                pc += printi (out, out_end, va_arg( args, int ), 16, 0, width, pad, 'a');
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("xxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }
            if( *format == 'X' ) {
                pc += printi (out, out_end, va_arg( args, int ), 16, 0, width, pad, 'A');
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("Xxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }
            if( *format == 'u' ) {
                pc += printi (out, out_end, va_arg( args, int ), 10, 0, width, pad, 'a');
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("uxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }
            if( *format == 'c' ) {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char)va_arg( args, int );
                scr[1] = '\0';
                pc += prints (out, out_end, scr, width, pad);
                #ifdef CHECK_PRINT_RET
                #if 1
                ASSERT((out == NULL) || ((*out - out_start) == pc));
                #else
                if ((out != NULL) && ((*out - out_start) != pc))
                    putstr("cxx\n", 4);
                #endif // 1
                #endif // CHECK_PRINT_RET
                continue;
            }  /*lint -restore */
        }
        else {
            out:
            printchar (out, out_end, *format);
            ++pc;
            #ifdef CHECK_PRINT_RET
            #if 1
            ASSERT((out == NULL) || ((*out - out_start) == pc));
            #else
            if ((out != NULL) && ((*out - out_start) != pc))
                putstr("+xx\n", 4);
            #endif // 1
            #endif // CHECK_PRINT_RET
        }
    }
    /*lint -restore */
    if (out) **out = '\0';
    return pc;   
}

#endif /* CONFIG_SIM_PLATFORM */



