#ifndef _SSV_LIB_H_
#define _SSV_LIB_H_

#include <config.h>
#include <regs.h>

#if (CONFIG_SIM_PLATFORM == 0)
#include <stdarg.h>
#endif


LIB_APIs u32 ssv6xxx_atoi_base( const s8 *s, u32 base );
LIB_APIs s32 ssv6xxx_atoi( const s8 *s );
LIB_APIs s32 ssv6xxx_isalpha(s32 c);
LIB_APIs s32 ssv6xxx_str_tolower(s8 *s);
LIB_APIs s32 ssv6xxx_str_toupper(s8 *s);

LIB_APIs s32 ssv6xxx_strrpos(const char *str, char delimiter);

#if (CONFIG_SIM_PLATFORM == 0)
LIB_APIs s8 toupper(s8 ch);
LIB_APIs s8 tolower(s8 ch);


LIB_APIs s32 strcmp( const s8 *s0, const s8 *s1 );
LIB_APIs s8 *strcat(s8 *s, const s8 *append);
LIB_APIs s8 *strncpy(s8 *dst, const s8 *src, size_t n);
LIB_APIs size_t strlen(const s8 *s);


LIB_APIs void *memset(void *s, s32 c, size_t n);
LIB_APIs void *memcpy(void *dest, const void *src, size_t n);
LIB_APIs s32 memcmp(const void *s1, const void *s2, size_t n);

LIB_APIs void *malloc(size_t size);
LIB_APIs void free(void *ptr);

LIB_APIs s32 vsnprintf(char *out, size_t size, const char *format, va_list args);
LIB_APIs s32 snprintf(char *out, size_t size, const char *format, ...);
LIB_APIs s32 printf(const s8 *format, ...);

LIB_APIs s32 putstr(const s8 *str, size_t size);
LIB_APIs s32 snputstr(char *out, size_t size, const s8 *str, size_t len);

LIB_APIs s32 fatal_printf(const s8 *format, ...);
#else
#define fatal_printf		printf
#endif

#if (CLI_ENABLE==1 && CONFIG_SIM_PLATFORM==0)
LIB_APIs s32 kbhit(void);
LIB_APIs s32 getch(void);
LIB_APIs s32 putchar(s32 ch);
#endif

LIB_APIs void ssv6xxx_raw_dump(s8 *data, s32 len);

// with_addr : (true) -> will print address head "xxxxxxxx : " in begining of each line
// addr_radix: 10 (digial)  -> "00000171 : "
//		     : 16 (hex)		-> "000000ab : "
// line_cols : 8, 10, 16, -1 (just print all in one line)
// radix     : 10 (digital) ->  171 (max num is 255)
//			   16 (hex)		-> 0xab
// log_level : log level  pass to LOG_PRINTF_LM()
// log_module: log module pass to LOG_PRINTF_LM()
//
LIB_APIs bool ssv6xxx_raw_dump_ex(s8 *data, s32 len, bool with_addr, u8 addr_radix, s8 line_cols, u8 radix, u32 log_level, u32 log_module);
LIB_APIs void scan_packet_buffer();
LIB_APIs void hex_dump(const void *addr, u32 size);
LIB_APIs void hex_parser(u32 rx_data);

LIB_APIs void halt(void);


#endif /* _SSV_LIB_H_ */

