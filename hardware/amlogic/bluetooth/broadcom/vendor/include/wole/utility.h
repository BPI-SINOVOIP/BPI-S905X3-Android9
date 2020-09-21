#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define HCI_VSC_WRITE_RAM                       0xFC4C
#define HCI_VSC_READ_RAM                        0xFC4D
#define WRITE_RAM_ADDR_MANUFACTURE_PATTERN      0xFF002005
#define READ_RAM_POLL_ADDRESS                   0xFF002006

void* wole_vsc_write_thread( void *ptr);
void wole_config_write_manufacture_pattern(void);
void wole_config_start();
