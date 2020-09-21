/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

/* The following features can be controlled:
 *
 * FREESAT_DATA_DIRECTORY   - Directory where the freesat.tX files are stored
 * FREESAT_ARCHIVE_MESSAGES - Store raw huffman strings in /var/tmp/
 * FREESAT_PRINT_MISSED_DECODING - Append missed hexcodes to end of string
 * FREESAT_NO_SYSLOG             - Disable use of isyslog
 */

//#include "freesat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <am_debug.h>


#define RELOAD_TIME     600      /* Minimum time between checking for updated tables */
#ifndef FREESAT_DATA_DIRECTORY
#define FREESAT_DATA_DIRECTORY       "/data/"
#endif

#if 0
#define TABLE1_FILENAME FREESAT_DATA_DIRECTORY "freesat.t1"
#define TABLE2_FILENAME FREESAT_DATA_DIRECTORY "freesat.t2"
#endif

#include "freesat_t1.h"
#include "freesat_t2.h"

//#define FREESAT_PRINT_MISSED_DECODING

struct hufftab {
    unsigned int value;
    short bits;
    char next;
};

#define START   '\0'
#define STOP    '\0'
#define ESCAPE  '\1'


int freesat_decode_error = 0;  /* If set an error has occurred during decoding */

static struct hufftab *tables[2][256];
static int             table_size[2][256];
static time_t          load_time = 0;

static void load_file(int tableid, const char *input);


static void freesat_table_load()
{
    struct stat sb;
    time_t      now = time(NULL);
    int         i;


    /* Only reload if we've never loaded, or it's more than 10 minutes away */
    if ( load_time != 0 ) {
#if 0
        int reload = 1;
        if ( now - load_time < RELOAD_TIME ) {
            return;
        }
        if ( stat(TABLE1_FILENAME,&sb) == 0 ) {
            if ( sb.st_mtime < load_time ) {
                if ( stat(TABLE2_FILENAME,&sb) == 0 ) {
                    if ( sb.st_mtime < load_time ) {
                        reload = 0;
                    }
                }
            }
        }
        if ( reload == 0 ) {
            return;
        }
#else
		return;
#endif
    }

    /* Reset the load time */
    load_time = now;


    /* Reset all the tables */
    for ( i = 0 ; i < 256; i++ ) {
        if ( tables[0][i] != NULL ) {
            free(tables[0][i]);
        }
        if ( tables[1][i] ) {
            free(tables[1][i]);
        }
        tables[0][i] = NULL;
        tables[1][i] = NULL;
        table_size[0][i] = 0;
        table_size[1][i] = 0;
    }

    /* And load the files up */
    load_file(1, freesat_table1);
    load_file(2, freesat_table2);
}


/** \brief Convert a textual character description into a value
 *
 *  \param str - Encoded (in someway) string
 *
 *  \return Raw character
 */
static unsigned char resolve_char(char *str)
{
    int val;
    if ( strcmp(str,"ESCAPE") == 0 ) {
        return ESCAPE;
    } else if ( strcmp(str,"STOP") == 0 ) {
        return STOP;
    } else if ( strcmp(str,"START") == 0 ) {
        return START;
    } else if ( sscanf(str,"0x%02x", &val) == 1 ) {
        return val;
    }
    return str[0];


}


/** \brief Decode a binary string into a value
 *
 *  \param binary - Binary string to decode
 *
 *  \return Decoded value
 */
static unsigned long decode_binary(char *binary)
{
    unsigned long mask = 0x80000000;
    unsigned long maskval = 0;
    unsigned long val = 0;
    size_t i;

    for ( i = 0; i < strlen(binary); i++ ) {
        if ( binary[i] == '1' ) {
            val |= mask;
        }
        maskval |= mask;
        mask >>= 1;
    }
    return val;
}

static char*
input_gets(const char *input, int *ppos, char *buf)
{
	const char *ptr;
	int pos = *ppos;
	int cnt;

	if (input[pos] == 0)
		return NULL;

	ptr = strchr(input + pos, '\n');
	if (ptr) {
		cnt = ptr - input - pos + 1;
	}else{
		cnt = strlen(input + pos);
	}

	memcpy(buf, input + pos, cnt);
	*ppos = pos + cnt;

	return buf;
}

/** \brief Load an individual freesat data file
 *
 *  \param tableid   - Table id that should be loaded
 *  \param input     - Input data buffer
 */
static void load_file(int tableid, const char *input)
{
    char     buf[1024];
	int      pos = 0;
    //char    *from, *to, *binary;

    tableid--;

	AM_DEBUG(1,"Loading table %d",tableid + 1);

	while ( input_gets(input, &pos, buf) != NULL ) {
		//from = binary = to = NULL;
		char from[128]={0};
		char to[128]={0};
		char binary[128]={0};


		int elems = sscanf(buf,"%[^:]:%[^:]:%[^:]:",from, binary, to);

		//AM_DEBUG(1,"--elems=%d,tableid=%d,buf=%s-\n",elems,tableid,buf);
		//AM_DEBUG(1,"--from=%s,binary=%s,to=%s\n",from,binary,to);
	   if ( elems == 3 ) {
			int bin_len = strlen(binary);
			int from_char = resolve_char(from);
			char to_char = resolve_char(to);
			unsigned long bin = decode_binary(binary);
			int i = table_size[tableid][from_char]++;

			tables[tableid][from_char] = (struct hufftab *)realloc(tables[tableid][from_char], (i+1) * sizeof(tables[tableid][from_char][0]));
			tables[tableid][from_char][i].value = bin;
			tables[tableid][from_char][i].next = to_char;
			tables[tableid][from_char][i].bits = bin_len;

		   // free(from);
		   // free(to);
		   // free(binary);
		}
	}
}

#ifdef FREESAT_ARCHIVE_MESSAGES
static void write_message(const unsigned char *src, size_t size)
{
    static FILE *fp = NULL;
    static int  written = 0;
    static int  suffix = 0;
    char buf[32768];

    if ( written > 10000 ) {
        fclose(fp);
        fp = NULL;
        written = 0;
    }
    if ( fp == NULL ) {
        snprintf(buf,sizeof(buf),"mkdir -p /data/tmp/%d",getpid());
        system(buf);

        snprintf(buf,sizeof(buf),"/data/tmp/%d/freesat.%d",getpid(),suffix);
        fp = fopen(buf, "w");
        suffix = (suffix + 1)%1000;
    }
    size_t offs = 0;
    size_t i;
    for (  i = 0; i < size; i++ ) {
        offs += snprintf(buf + offs, sizeof(buf) - offs, "%02x", src[i]);
    }
    fprintf(fp, "%s\n",buf);
    fflush(fp);
    written++;
}
#endif

/** \brief Decode an EPG string as necessary
 *
 *  \param src - Possibly encoded string
 *  \param size - Size of the buffer
 *
 *  \retval NULL - Can't decode
 *  \return A decoded string
 */
char  *freesat_huffman_decode( const unsigned char *src, size_t size)
{
    int tableid;

    freesat_decode_error = 0;

#ifdef FREESAT_ARCHIVE_MESSAGES
    write_message(src, size);
#endif
    if (src[0] == 0x1f && (src[1] == 1 || src[1] == 2)) {
        int    uncompressed_len = 30;
        char * uncompressed = (char *)calloc(1,uncompressed_len + 1);
        unsigned value = 0, byte = 2, bit = 0;
        int p = 0;
        int lastch = START;

        tableid = src[1] - 1;
        while (byte < 6 && byte < size) {
            value |= src[byte] << ((5-byte) * 8);
            byte++;
        }

        freesat_table_load();   /**< Load the tables as necessary */

        do {
            int found = 0;
            unsigned bitShift = 0;

            if (lastch == ESCAPE) {
                char nextCh = (value >> 24) & 0xff;
                found = 1;
                // Encoded in the next 8 bits.
                // Terminated by the first ASCII character.
                bitShift = 8;
                if ((nextCh & 0x80) == 0)
                    lastch = nextCh;
                if (p >= uncompressed_len) {
                    uncompressed_len += 10;
                    uncompressed = (char *)realloc(uncompressed, uncompressed_len + 1);
                }
                uncompressed[p++] = nextCh;
                uncompressed[p] = 0;
            } else {
                int j;
                for ( j = 0; j < table_size[tableid][lastch]; j++) {
                    unsigned mask = 0, maskbit = 0x80000000;
                    short kk;
                    for ( kk = 0; kk < tables[tableid][lastch][j].bits; kk++) {
                        mask |= maskbit;
                        maskbit >>= 1;
                    }
                    if ((value & mask) == tables[tableid][lastch][j].value) {
                        char nextCh = tables[tableid][lastch][j].next;
                        bitShift = tables[tableid][lastch][j].bits;
                        if (nextCh != STOP && nextCh != ESCAPE) {
                            if (p >= uncompressed_len) {
                                uncompressed_len += 10;
                                uncompressed = (char *)realloc(uncompressed, uncompressed_len + 1);
                            }
                            uncompressed[p++] = nextCh;
                            uncompressed[p] = 0;
                        }
                        found = 1;
                        lastch = nextCh;
                        break;
                    }
                }
            }
            if (found) {
                // Shift up by the number of bits.

                unsigned b;
                for ( b = 0; b < bitShift; b++)
                {
                    value = (value << 1) & 0xfffffffe;
                    if (byte < size)
                        value |= (src[byte] >> (7-bit)) & 1;
                    if (bit == 7)
                    {
                        bit = 0;
                        byte++;
                    }
                    else bit++;
                }
            } else {
#ifdef FREESAT_PRINT_MISSED_DECODING
                char  temp[1020];
                size_t   tlen = 0;

                tlen = snprintf(temp,sizeof(temp),"...[%02x][%02x][%02x][%02x]",(value >> 24 ) & 0xff, (value >> 16 ) & 0xff, (value >> 8) & 0xff, value &0xff);
                do {
                    // Shift up by the number of bits.
                    unsigned b;
                    for ( b = 0; b < 8; b++) {
                        value = (value << 1) & 0xfffffffe;
                        if (byte < size)
                            value |= (src[byte] >> (7-bit)) & 1;
                        if (bit == 7) {
                            bit = 0;
                            byte++;
                        }
                        else bit++;
                    }
                    tlen += snprintf(temp+tlen, sizeof(temp) - tlen,"[%02x]", value & 0xff);
                } while ( tlen < sizeof(temp) - 6 && byte <  size);

                uncompressed_len += tlen;
                uncompressed = (char *)realloc(uncompressed, uncompressed_len + 1);
                freesat_decode_error = 1;
                strcpy(uncompressed + p, temp);
#endif
                AM_DEBUG(1,"Missing table %d entry: <%s>",tableid + 1, uncompressed);
                // Entry missing in table.
                return uncompressed;
            }
        } while (lastch != STOP && value != 0);

        return uncompressed;
    }
    return NULL;
}
