#include <utils/Log.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_brcm.h"
#include "wole/utility.h"

//
//User should define the manufacture specific data that can support wake up feature below.
//=========================================================================
const uint8_t WOLE_MANUFACTURE_PATTERN[] = {0xff,0xff,0x41,0x6d,0x6c,0x6f,0x67,0x69,0x63};  //amlogic rc(Amlogic_RC20)
//const uint8_t WOLE_MANUFACTURE_PATTERN[] = {0x5d,0x00,0x03,0x00,0x01};  //Innopia
//=========================================================================

#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < 6;  ijk++) *(p)++ = (uint8_t) a[6 - 1 - ijk];}
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define HCI_CMD_MAX_LEN                         258
#define HCI_CMD_PREAMBLE_SIZE                   3
#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE        5
#define HCI_EVT_CMD_CMPL_OPCODE                 3


pthread_mutex_t s_vsclock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t s_vsccond = PTHREAD_COND_INITIALIZER;
int wake_signal_sent = 0;
//static int counter=0;



void thread_exit_handler(int sig)
{
    ALOGE("signal %d caught",sig);
    pthread_exit(0);
}

void* wole_vsc_write_thread( void *ptr)
{
    (void) ptr;
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR1,&actions,NULL);

    pthread_detach(pthread_self());
    pthread_mutex_lock(&s_vsclock);
    pthread_cond_wait(&s_vsccond, &s_vsclock);
    pthread_mutex_unlock(&s_vsclock);
    //to prevent the the command is sent before HCI_RESET which sent from stack
    usleep(200000);
    wole_config_write_manufacture_pattern();
    while (1)
    {
        //poll every seconds
        usleep(800000);

        //if(counter < 50 ) //for stimulate suspend status
        //{
        pthread_mutex_lock(&s_vsclock);
        wole_config_start();
        while (wake_signal_sent == 0)
            pthread_cond_wait(&s_vsccond, &s_vsclock);
        wake_signal_sent = 0;
        pthread_mutex_unlock(&s_vsclock);

        //}
        //counter++;
    }
}

/*******************************************************************************
**
** Function         wole_config_cback
**
** Description      WoLE Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void wole_config_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    //char        *p_name, *p_tmp;
    uint8_t     *p, status,wole_status;
    uint16_t    opcode;
    //HC_BT_HDR  *p_buf=NULL;
    //uint8_t     is_proceeding = FALSE;
    //int         i;
    //int         delay=100;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode,p);
    ALOGI("%s, status = %d, opcode=0x%x",__FUNCTION__,status,opcode);

    if (status == 0) //command is supported and everything is fine.
    {
        if (opcode == HCI_VSC_READ_RAM)
        {
            wole_status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1);
            ALOGI("%s, opcode read RAM, wole_status = %d",__FUNCTION__, wole_status);
            if (wole_status != 0)
            {
                kill(getpid(),SIGKILL);
            }
        }
        pthread_mutex_lock(&s_vsclock);
        wake_signal_sent=1;
        pthread_cond_signal(&s_vsccond);
        pthread_mutex_unlock(&s_vsclock);

    }


    /* Free the RX event buffer */
    if (bt_vendor_cbacks)
        bt_vendor_cbacks->dealloc(p_evt_buf);

}

void wole_config_write_manufacture_pattern(void)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;
    int i;

    ALOGI("%s",__FUNCTION__);

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE + 4 + sizeof(WOLE_MANUFACTURE_PATTERN) + sizeof(WOLE_MANUFACTURE_PATTERN);;  //Address is 4bytes

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p,HCI_VSC_WRITE_RAM);
        *p++ = 4 + sizeof(WOLE_MANUFACTURE_PATTERN) + sizeof(WOLE_MANUFACTURE_PATTERN);
        UINT32_TO_STREAM(p,WRITE_RAM_ADDR_MANUFACTURE_PATTERN);
        for (i=0;i< (int)sizeof(WOLE_MANUFACTURE_PATTERN);i++)
            *p++ = WOLE_MANUFACTURE_PATTERN[i];
        for (i=0;i< (int)sizeof(WOLE_MANUFACTURE_PATTERN);i++)
        {
            if (i == sizeof(WOLE_MANUFACTURE_PATTERN)-1)
                *p = 0xFF;
            else
                *p++ = 0xFF;
        }

        bt_vendor_cbacks->xmit_cb(HCI_VSC_WRITE_RAM, p_buf, wole_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib wole conf aborted [no buffer]");
        }
    }
}

/*******************************************************************************
**
** Function        wole_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void wole_config_start()
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;
    //int i;

    /* Start from sending HCI_RESET */
    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE+5;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p,HCI_VSC_READ_RAM);
        *p++ = 5; /* parameter length */
        UINT32_TO_STREAM(p,READ_RAM_POLL_ADDRESS);
        *p = 1; /* parameter length */

        bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_RAM, p_buf, wole_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib wole conf aborted [no buffer]");
        }
    }
}

