/****************************************************************************

               (c) Cambridge Silicon Radio Limited 2013

               All rights reserved and confidential information of CSR

REVISION:      $Revision: #1 $
****************************************************************************/
#define LOG_TAG "bt_vendor_qcom"
#include <unistd.h>
#include <sys/uio.h>
#include <utils/Log.h>
#include "bt_vendor_lib.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define DEFAULT_BT_USB_DEVICE_NAME  "/dev/bt_usb0"
int usb_fd;

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    ALOGD("init");
    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    memcpy(vnd_local_bd_addr, local_bdaddr, 6);
    return 0;
}

int usb_vendor_reset()
{
    ALOGD("usb_vendor_reset");
    int bt_usb_fd = -1;
    int count;
	(void)count;
    char devName[255] = DEFAULT_BT_USB_DEVICE_NAME;

    uint8_t BcCmdReset[22] = { 0x00, 0xFC, 0x13, 0xC2, 0x02, 0x00, 0x09, 
    	                                        0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                             };
    
/*create the node and link to device*/

    bt_usb_fd = open(devName, O_NONBLOCK|O_RDWR);
    if (bt_usb_fd == -1)
    {
        ALOGE("could not open usb device btusb0");
    	return -1;
    }

    uint8_t channel = 5;
    struct iovec out[2];
    out[0].iov_base = &channel;
    out[0].iov_len  = 1;
    out[1].iov_base = BcCmdReset;
    out[1].iov_len  = 22;

    writev(bt_usb_fd,out,2);      
    usleep(4000000); //TODO - should be event based
    
    if (bt_usb_fd != -1)
    {
	close(bt_usb_fd);
    }
    bt_usb_fd = -1;
    usb_fd = -1;

    return 1;
}

void usb_vendor_close()
{
    ALOGD("qcom usb_vendor_close %d",usb_fd);
#ifdef CSR_SUPPORT_HID_SWITCH_CMD
    /* resetting the chip on bt off, so that it re-enumerates as HID device */
    int bt_usb_fd = -1;
    char devName[255] = DEFAULT_BT_USB_DEVICE_NAME;

    uint8_t BcCmdReset[22] = { 0x00, 0xFC, 0x13, 0xC2, 0x02, 0x00, 0x09, 
    	                                        0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                             };
    
    bt_usb_fd = open(devName, O_NONBLOCK|O_RDWR);
    if (bt_usb_fd == -1)
    {
        ALOGE("could not open usb device %s",devName);
    }
    else
    {
        uint8_t channel = 5;
        struct iovec out[2];
        out[0].iov_base = &channel;
        out[0].iov_len  = 1;
        out[1].iov_base = BcCmdReset;
        out[1].iov_len  = 22;

        writev(bt_usb_fd,out,2);      
    }
#endif
    if(usb_fd > 0)
        close(usb_fd);
    usb_fd=-1;
}

int usb_vendor_open()
{
    ALOGD("qcom usb_vendor_open");
   
    int bt_usb_fd = -1;
    int i;
    char devName[255] = DEFAULT_BT_USB_DEVICE_NAME;

#ifdef CSR_SUPPORT_HID_SWITCH_CMD       
    int fd;
    unsigned int data = 0x0000; 
    /* you may have just reset the chip. It takes couple of seconds to 
       enumerate in hid mode*/
    for(i=0 ; i< 5 ; i++ )
    {
        if((fd = open ("/dev/hid-switch", O_NONBLOCK|O_RDWR) ) > 0)
        {
            ALOGD("bluetooth init: Switching to HCI mode");
            write(fd, &data, sizeof(data)); 
            close(fd);
            break;
        }
        ALOGD("open /dev/hid-swich failed, error %d attempt:%d", errno,i);
        usleep(500000);
    }
#endif
    for(i=0; i< 1000; i++)
    {
        bt_usb_fd = open(devName, O_NONBLOCK|O_RDWR);    
        if(bt_usb_fd > 0)
        {
            usb_fd = bt_usb_fd;
            ALOGD(" usb device opened %d",bt_usb_fd);
            break;
        }
#ifdef CSR_SUPPORT_HID_SWITCH_CMD       
        usleep(500000);/* in case of hid->hci switch more time might take for enumeration */
#else
        usleep(100000);
#endif
        ALOGE("could not open %s, err %d, errno %d, trying again (attempt:%d)", devName, bt_usb_fd,errno, i);
    }
    return bt_usb_fd;
}

static int op(bt_vendor_opcode_t opcode, void *param)
{
	int retval = 0;

	ALOGD("op for %d", opcode);

	switch(opcode)
	{
	case BT_VND_OP_POWER_CTRL:
		{
			ALOGD("op for BT_VND_OP_POWER_CTRL");
		}
		break;

	case BT_VND_OP_FW_CFG:
		{
			ALOGD("op for BT_VND_OP_FW_CFG");
			if(bt_vendor_cbacks){
				bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);//dummy
			}
		}
		break;

	case BT_VND_OP_SCO_CFG:
		{
			ALOGD("op for BT_VND_OP_SCO_CFG");
			if(bt_vendor_cbacks){
				bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
			}	
		}
		break;

	case BT_VND_OP_USERIAL_OPEN:
		{
			int (*fd_array)[] = (int (*)[]) param;
			int fd, idx;
			
			ALOGD("op for BT_VND_OP_USERIAL_OPEN");

			fd = usb_vendor_open();

			if (fd != -1)
			{
				for (idx=0; idx < CH_MAX; idx++)
				{
					(*fd_array)[idx] = fd;
				}
				retval = 1;
			}
		}
		break;

	case BT_VND_OP_USERIAL_CLOSE:
		{
			ALOGD("op for BT_VND_OP_USERIAL_CLOSE");
			usb_vendor_close();
		}
		break;

	case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
		{
			ALOGD("op for BT_VND_OP_GET_LPM_IDLE_TIMEOUT *param = 3000");
			*((uint32_t *)param) = 3000;	
		}
		break;

	case BT_VND_OP_LPM_SET_MODE:
		{
			ALOGD("op for BT_VND_OP_LPM_SET_MODE");
			if(bt_vendor_cbacks){			
				bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
			}	
		}
	break;

	case BT_VND_OP_LPM_WAKE_SET_STATE:
		{
			ALOGD("op for BT_VND_OP_LPM_WAKE_SET_STATE");
		}	
		break;
		
	case BT_VND_OP_EPILOG:
		{
			ALOGD("op for BT_VND_OP_EPILOG");
			if(bt_vendor_cbacks){
				bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);//dummy
			}
		}	
		break;

	default:
		break;
	}

	return retval;
}

static void cleanup( void )
{
    ALOGD("cleanup");
    if(usb_fd > 0)
    {
        close(usb_fd);
        usb_fd = -1;
    }
    bt_vendor_cbacks = NULL;
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};
