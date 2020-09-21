#include <config.h>
#include <mbox/drv_mbox.h>
#include <timer/drv_timer.h>
#include <bsp/irq.h>
#include <log.h>
#include <./../drv_comm.h>
#include <pbuf.h>
#include <ssv_pktdef.h>
#include <hdr80211.h>
#include "drv_ampdu.h"
#include <hwmac/drv_mac.h>

//#define DUMP_AMPDU_TX_DESC
//#define AMPDU_HAS_LEADING_FRAME
#define MAX_CONCURRENT_AMPDU        16
#define skb_ssn(hdr)        (((hdr)->seq_ctrl)>>4)

static volatile u32 current_waiting_BA_idx = MAX_CONCURRENT_AMPDU;

#define PRINTF(fmt, ...) \
    do { \
        printf(fmt, ##__VA_ARGS__); \
        CHECK_DEBUG_INT(); \
    } while (0)

#ifdef DUMP_AMPDU_TX_DESC
bool to_dump_first_retry_tx_desc = true;
#endif // DUMP_AMPDU_TX_DESC

// IEEE802.11 frame header of block ack.
typedef struct AMPDU_BLOCKACK_st
{
	u16			frame_control;
	u16			duration;
	u8			ra_addr[6];
	u8			ta_addr[6];

    u16         BA_ack_ploicy:1;
    u16         multi_tid:1;
    u16         compress_bitmap:1;
    u16         reserved:9;
    u16         tid_info:4;

    u16         BA_fragment_sn:4;
    u16         BA_ssn:12;
    u32         BA_sn_bit_map[2];
} __attribute__((packed)) AMPDU_BLOCKACK, *p_AMPDU_BLOCKACK;

struct AMPDU_TX_Data_S {
    u32              tx_pkt;
#ifdef AMPDU_HAS_LEADING_FRAME
    u32              leading_pkt;
#endif
    u16              retry_count;
    u16              rates_idx;
    struct ssv62xx_tx_rate tried_rates[SSV62XX_TX_MAX_RATES];
    u16              retry_end;
};

static struct AMPDU_TX_Data_S ampdu_tx_data[MAX_CONCURRENT_AMPDU] = {{0}};

static void _retry_ampdu_frame (struct AMPDU_TX_Data_S *p_ampdu_tx_data);
//static bool _proc_ampdu_tx_pkt (u32 ampdu_tx_pkt);
static u32 _notify_host_BA_drop (u32 retry_count, u32 tx_data, struct ssv62xx_tx_rate* record);
static void _appand_ba_notify (u32 rx_data, u32 tx_data, u32 retry_count, struct ssv62xx_tx_rate* record);
static void _reset_ampdu_mib (void);

#ifdef AMPDU_HAS_LEADING_FRAME
static u32 _create_null_frame_for_ampdu (u32 tx_frame);
#endif 


#ifdef ENEABLE_MEAS_AMPDU_TX_TIME
#include <dbg_timer/dbg_timer.h>
static Time_T ampdu_tx_report_time;
u32 max_ba_elapsed_time = 0;
u32 total_ba_elapsed_time = 0;
u32 ba_elapsed_time_count = 0;
u32 small_timeout_count = 0;
u32 min_timeout_elapsed_time = (u32)(-1);
#endif //ENEABLE_MEAS_AMPDU_TX_TIME

// AMPDU TX statistics
u32    ampdu_st_reset = 0;
u32    ampdu_tx_count = 0;
u32    ampdu_tx_1st_count = 0;
u32    ampdu_tx_retry_count = 0;
u32    ampdu_tx_force_drop_count = 0;
u32    ampdu_tx_retry_drop_count = 0;
u32    ampdu_tx_drop_count = 0;
u32    ampdu_tx_ba_count = 0;
u32    ampdu_tx_1st_ba_count = 0;
u32    ampdu_tx_retry_ba_count = 0;


static inline void us_timer_start(HW_TIMER *tmr, u32 count, u32 data, enum hwtmr_op_mode mode)
{
    //ampdu_fw_timer_data = data;
    tmr->TMR_CTRL = HTMR_COUNT_VAL(count)|HTMR_OP_MODE(mode);
}


static void us_timer_stop (HW_TIMER *tmr)
{
    tmr->TMR_CTRL = TU0_TM_INT_STS_DONE_MSK;
}


static inline void ssv6200_ampdu_free_pkt_and_stop_timer(struct AMPDU_TX_Data_S *p_ampdu_tx_data, HW_TIMER *tmr)
{
    us_timer_stop(tmr);
    
    current_waiting_BA_idx = MAX_CONCURRENT_AMPDU;

    if(p_ampdu_tx_data->tx_pkt != 0)
    {
        ENG_MBOX_SEND(M_ENG_TRASH_CAN,(u32)p_ampdu_tx_data->tx_pkt);
        //PBUF_MFree((void *)rx_data);
    }
	#ifdef AMPDU_HAS_LEADING_FRAME
    if(p_ampdu_tx_data->leading_pkt != 0)
    {
        ENG_MBOX_SEND(M_ENG_TRASH_CAN,(u32)p_ampdu_tx_data->leading_pkt);
        p_ampdu_tx_data->leading_pkt = 0;
        //PBUF_MFree((void *)rx_data);
    }
	#endif // AMPDU_HAS_LEADING_FRAME
    memset(p_ampdu_tx_data, 0, sizeof(struct AMPDU_TX_Data_S));
}

static inline bool ssv6200_ampdu_is_ctrl_frame_pkt(u32 rx_data)
{
    struct ssv6200_rx_desc_from_soc *rx_desc_from_soc;
    u8 MAC_header;
    
    rx_desc_from_soc = (struct ssv6200_rx_desc_from_soc *)rx_data;
    MAC_header = *(u8 *)(rx_data + GET_PB_OFFSET);

    return (((((rx_desc_from_soc->fCmd)&0xf) == M_ENG_MACRX))&&((MAC_header & 0xc) == 0x4))? 1: 0;
}

#ifdef AMPDU_HAS_LEADING_FRAME
static inline bool ssv6200_ampdu_is_tx_null_frame_pkt(u32 tx_data)
{
	struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)tx_data;
	u16 fc = *(u16 *)(tx_data + GET_PB_OFFSET);

    return ((((tx_desc->fCmd)&0xf) == M_ENG_HWHCI) && ((fc & (FST_NULLFUNC | FT_DATA)) == (FST_NULLFUNC | FT_DATA)));
}
#endif // AMPDU_HAS_LEADING_FRAME

static inline bool ssv6200_ampdu_is_BA_pkt(u32 fcmd, u8 MAC_header)
{
    return (((fcmd & 0xf) == M_ENG_MACRX)&&((MAC_header & 0xfc) == 0x94))? 1: 0;
}

static inline bool ssv6200_ampdu_is_ampdu_tx_pkt(u32 rx_data)
{
    struct ssv6200_tx_desc *tx_desc;

    tx_desc = (struct ssv6200_tx_desc *)rx_data;

    return ((((tx_desc->fCmd)&0xf) == M_ENG_HWHCI)&&(tx_desc->aggregation == 1))? 1: 0;
}


static inline AMPDU_DELIMITER *get_ampdu_delim_from_ssv_pkt (u32 rx_data)
{
    u32 TX_PB_offset = GET_TX_PBOFFSET;
//    struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc*)rx_data;
    AMPDU_DELIMITER *tx_delimiter = (AMPDU_DELIMITER *)(rx_data + TX_PB_offset);
    return tx_delimiter;
}


static inline struct ssv6200_80211_hdr *get_80211hdr_from_delim (AMPDU_DELIMITER *tx_delimiter)
{
    struct ssv6200_80211_hdr *hdr = (struct ssv6200_80211_hdr*)((u32)tx_delimiter + SSV6200_AMPDU_DELIMITER_LENGTH);
    return hdr;
}


static inline AMPDU_DELIMITER *get_next_delim (AMPDU_DELIMITER *tx_delimiter)
{
    AMPDU_DELIMITER *next_tx_delimiter;
    u32 mpdu_len = tx_delimiter->length;
    mpdu_len += ((4-(mpdu_len%4))%4);             // add padding length
    mpdu_len += SSV6200_AMPDU_DELIMITER_LENGTH;    // add delimiter length

    next_tx_delimiter = (AMPDU_DELIMITER *)((u32)tx_delimiter + mpdu_len);
    return next_tx_delimiter;
}


static inline void prt_ssn (const char *s, u32 tx_pkt)
{
    AMPDU_DELIMITER *delim = get_ampdu_delim_from_ssv_pkt(tx_pkt);
    struct ssv6200_80211_hdr *hdr = get_80211hdr_from_delim(delim);
    PRINTF("%s %d\n", s, skb_ssn(hdr));
}


static inline void ssv6200_ampdu_set_retry_bit(u32 rx_data)
{
    struct ssv6200_tx_desc *tx_desc;
    AMPDU_DELIMITER *tx_delimiter;
    struct ssv6200_80211_hdr *hdr;
    u16 mpdu_total_len;
    u16 mpdu_len;
    u16 current_len;
    u16 TX_PB_offset;
    
    TX_PB_offset = GET_TX_PBOFFSET;
    tx_desc = (struct ssv6200_tx_desc*)rx_data;
    tx_delimiter = (AMPDU_DELIMITER *)(rx_data + TX_PB_offset);
    mpdu_total_len = tx_desc->len;
    current_len = 0;

    //printf("mpdu_total_len = %d \n", mpdu_total_len);  
    //printf("TX_PB_offset = %d \n", TX_PB_offset);  

    while(1)
    {
        //printf("tx_delimiter = 0x%x \n", *(u32 *)tx_delimiter); 

        hdr = (struct ssv6200_80211_hdr*)((u32)tx_delimiter + SSV6200_AMPDU_DELIMITER_LENGTH);
        //printf("1. hdr = 0x%x, hdr->frame_ctl = 0x%x \n", *(u32 *)hdr, hdr->frame_ctl); 
        
        // set retry bit =========================
        hdr->frame_ctl |= (0x1<<SSV6200_RETRY_BIT_SHIFT);
        //printf("2. hdr = 0x%x, hdr->frame_ctl = 0x%x \n", *(u32 *)hdr, hdr->frame_ctl); 
    
        mpdu_len = tx_delimiter->length;
        mpdu_len += ((4-(mpdu_len%4))%4);             // add padding length
        mpdu_len += SSV6200_AMPDU_DELIMITER_LENGTH;    // add delimiter length
        //printf("mpdu_len = %d \n", mpdu_len);  

        current_len += mpdu_len;
        //printf("current_len(%d) = %d \n", mpdu_total_len, current_len);  
        
        if(current_len >= mpdu_total_len)
        {
            break;
        }

        tx_delimiter = (AMPDU_DELIMITER *)((u32)tx_delimiter + mpdu_len);
    }

    #ifdef DUMP_AMPDU_TX_DESC
    if (to_dump_first_retry_tx_desc)
    {
        hex_dump(tx_desc, sizeof(struct ssv6200_tx_desc));
        to_dump_first_retry_tx_desc = false;
    }
    #endif // DUMP_AMPDU_TX_DESC
}


void ampdu_fw_retry_init(void)
{
    current_waiting_BA_idx = MAX_CONCURRENT_AMPDU;
    memset(ampdu_tx_data, 0, sizeof(ampdu_tx_data));
    /* Register us_timer_0 interrupt handler here: */
    irq_request(IRQ_US_TIMER0, irq_us_timer0_handler, (void *)&current_waiting_BA_idx);
}


void irq_us_timer0_handler(void *m_data)
{
    struct AMPDU_TX_Data_S *p_ampdu_tx_data;

    if (current_waiting_BA_idx == MAX_CONCURRENT_AMPDU)
    {
        return;
    }

#ifdef ENEABLE_MEAS_AMPDU_TX_TIME
    {
    u32 timeout_elapsed_time = dbg_calc_elapsed(&ampdu_tx_report_time, &irq_time);
    if (timeout_elapsed_time < AMPDU_FW_retry_time_us)
        small_timeout_count++;
    if (min_timeout_elapsed_time > timeout_elapsed_time)
        min_timeout_elapsed_time = timeout_elapsed_time;
    }
#endif

    p_ampdu_tx_data = &ampdu_tx_data[current_waiting_BA_idx];

    current_waiting_BA_idx = MAX_CONCURRENT_AMPDU;
    if (p_ampdu_tx_data->retry_end == 0)
    {
        _retry_ampdu_frame(p_ampdu_tx_data);
        //prt_ssn("t", p_ampdu_tx_data->tx_pkt);
    }
    else
    {
        u32 tx_pkt;
        prt_ssn("T", p_ampdu_tx_data->tx_pkt);
        tx_pkt = _notify_host_BA_drop(p_ampdu_tx_data->retry_count, 
                                      p_ampdu_tx_data->tx_pkt, 
                                      p_ampdu_tx_data->tried_rates);
        if (tx_pkt != 0)
            ENG_MBOX_SEND(M_ENG_TRASH_CAN, (u32)p_ampdu_tx_data->tx_pkt);
        #ifdef AMPDU_HAS_LEADING_FRAME
        if (p_ampdu_tx_data->leading_pkt != 0)
        {
            ENG_MBOX_SEND(M_ENG_TRASH_CAN, (u32)p_ampdu_tx_data->leading_pkt);
            p_ampdu_tx_data->leading_pkt = 0;
        }
        #endif // AMPDU_HAS_LEADING_FRAME
        memset(p_ampdu_tx_data, 0, sizeof(struct AMPDU_TX_Data_S));
        ++ampdu_tx_retry_drop_count;
        ++ampdu_tx_drop_count;
    }
}


bool ampdu_rx_ctrl_handler (u32 rx_data)
{
    struct cfg_host_rxpkt *rx_desc_from_soc = (struct cfg_host_rxpkt *)rx_data;
    u32 fcmd = rx_desc_from_soc->fCmd;
    u8 *fc = (u8 *)(rx_data + GET_PB_OFFSET);
    p_AMPDU_BLOCKACK MAC_header = (p_AMPDU_BLOCKACK)fc;
    bool is_BA = ssv6200_ampdu_is_BA_pkt(fcmd, *fc);

    if (is_BA)
    {
        us_timer_stop(US_TIMER0);
        if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
        {
            // Check if the Received BA matches is from the destination of the waiting AMPDU.
            struct AMPDU_TX_Data_S *p_ampdu_tx_data = &ampdu_tx_data[current_waiting_BA_idx];
            AMPDU_DELIMITER *delim = get_ampdu_delim_from_ssv_pkt(p_ampdu_tx_data->tx_pkt);
            struct ssv6200_80211_hdr *hdr = get_80211hdr_from_delim(delim);
            //      BA TA                  != AMPDU RA
            if (   (MAC_header->ta_addr[0] != hdr->addr1[0])
                || (MAC_header->ta_addr[1] != hdr->addr1[1])
                || (MAC_header->ta_addr[2] != hdr->addr1[2])
                || (MAC_header->ta_addr[3] != hdr->addr1[3])
                || (MAC_header->ta_addr[4] != hdr->addr1[4])
                || (MAC_header->ta_addr[5] != hdr->addr1[5]))
            {
                current_waiting_BA_idx = MAX_CONCURRENT_AMPDU;
                if (p_ampdu_tx_data->retry_end != 1)
                {
                    _retry_ampdu_frame(p_ampdu_tx_data);
                    PRINTF("bt - %d\n", MAC_header->BA_ssn);
                }
                else
                {
                 u32 ssn = MAC_header->BA_ssn;
                    p_ampdu_tx_data->tx_pkt = _notify_host_BA_drop(p_ampdu_tx_data->retry_count,
                                                                 p_ampdu_tx_data->tx_pkt,
                                                                 p_ampdu_tx_data->tried_rates);
                    ssv6200_ampdu_free_pkt_and_stop_timer(p_ampdu_tx_data, US_TIMER0);
                    p_ampdu_tx_data->tx_pkt = 0;
                    p_ampdu_tx_data->retry_count = 0;
                    ++ampdu_tx_retry_drop_count;
                    ++ampdu_tx_drop_count;
                  memset(p_ampdu_tx_data, 0, sizeof(struct AMPDU_TX_Data_S));
                  PRINTF("bT - %d\n", ssn);
                }
                is_BA = false;
            }
            else
            {
                _appand_ba_notify(rx_data, ampdu_tx_data[current_waiting_BA_idx].tx_pkt,
                                  ampdu_tx_data[current_waiting_BA_idx].retry_count,
                                  ampdu_tx_data[current_waiting_BA_idx].tried_rates);
              //printf("B - %d - %08X %d %d %08X\n", MAC_header->BA_ssn, rx_desc_from_soc->fCmd, rx_desc_from_soc->fCmdIdx, rx_desc_from_soc->reason, rx_data);
              //printf("B %d\n", MAC_header->BA_ssn);
            }
        }
        else
        {
            PRINTF("b - %d - %08X %d %d %08X\n", MAC_header->BA_ssn, rx_desc_from_soc->fCmd, rx_desc_from_soc->fCmdIdx, rx_desc_from_soc->reason, rx_data);
        }
    }

    ENG_MBOX_NEXT(rx_data);

    if (is_BA)
    {
        #if 0
        printf("RX_PB_OFFSET = %d\n", GET_PB_OFFSET);

        // print all BA skb
        {
            u32 temp_i;
            u8* temp8_p = (u8*)rx_data;

            printf("print all BA skb.\n");

            for(temp_i=0 ; temp_i < (*(u16*)rx_data + GET_PB_OFFSET); temp_i++)
            {
                printf("%02x",*(u8*)(temp8_p + temp_i));

                if(((temp_i+1) % 4) == 0)
                {
                    printf("\n");
                }
            }
            printf("\n");
        }
        #endif // 0
        #ifdef ENEABLE_MEAS_AMPDU_TX_TIME
        if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
        {
            u32 ba_elapsed_time = dbg_calc_elapsed(&ampdu_tx_report_time, &irq_time);
            if (max_ba_elapsed_time < ba_elapsed_time)
                max_ba_elapsed_time = ba_elapsed_time;
            total_ba_elapsed_time += ba_elapsed_time;
            ba_elapsed_time_count++;
        }
        #endif // ENEABLE_MEAS_AMPDU_TX_TIME

        ++ampdu_tx_ba_count;
        if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
        {
            u32 idx = current_waiting_BA_idx;
            if (ampdu_tx_data[idx].retry_count != 0)
            {
                ++ampdu_tx_retry_ba_count;
            }
            else
            {
                ++ampdu_tx_1st_ba_count;
            }
            ssv6200_ampdu_free_pkt_and_stop_timer(&ampdu_tx_data[idx], US_TIMER0);
        }
        else
        { // Unlikely, BA received without waiting AMPDU frame.
            ++ampdu_tx_1st_ba_count;
            // printf("--\n");
        }
    }

    return true;
} // end of - ampdu_rx_ctrl_handler -


void _retry_ampdu_frame (struct AMPDU_TX_Data_S *p_ampdu_tx_data)
{
    struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc*)p_ampdu_tx_data->tx_pkt;
    struct fw_rc_retry_params *ptr = NULL;
    if (p_ampdu_tx_data->tx_pkt == 0)
        return;

    ptr = &tx_desc->rc_params[p_ampdu_tx_data->rates_idx];

    // For new AMPDU frame
    if (p_ampdu_tx_data->retry_count == 0)
        ssv6200_ampdu_set_retry_bit(p_ampdu_tx_data->tx_pkt);

    if(p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].count < ptr->count)
    {
        p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].count++;
        //printf("send count increases\n");
    }
    else if ((p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].count == ptr->count)
                && (p_ampdu_tx_data->rates_idx < (SSV62XX_TX_MAX_RATES - 1)))
    {
        ptr += 1;
        if(ptr->count)
        {
            p_ampdu_tx_data->rates_idx++;

            p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].count = 1;
            p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].data_rate = ptr->drate;

            tx_desc->crate_idx = ptr->crate;
            tx_desc->drate_idx = ptr->drate;
            tx_desc->rts_cts_nav = ptr->rts_cts_nav;
            tx_desc->frame_consume_time = ptr->frame_consume_time;
            tx_desc->dl_length = ptr->dl_length;    
            //printf("Update rate relate parameters\n");
        }
        else
        {
            p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx+1].data_rate = -1;
            p_ampdu_tx_data->retry_end = 1;
            //printf("Strange params for rate control, stop\n");
            return;
        }            
    }
    else
    {
        p_ampdu_tx_data->retry_end = 1;
        //printf("No more params for rate control\n");
        return;
    }
    
    if ((p_ampdu_tx_data->tried_rates[p_ampdu_tx_data->rates_idx].count == ptr->count)
                && (p_ampdu_tx_data->rates_idx == (SSV62XX_TX_MAX_RATES - 1)))
    {
        //printf("End retry\n");
        p_ampdu_tx_data->retry_end = 1;
    }
        
    p_ampdu_tx_data->retry_count++;
    // Debug code: is over retry?
    if (p_ampdu_tx_data->retry_count >= AMPDU_FW_retry_counter_max)
        prt_ssn("Ot", p_ampdu_tx_data->tx_pkt);

    //ENG_MBOX_SEND((((tx_desc->fCmd)>>4)&0xf), p_ampdu_tx_data->tx_pkt);
    #ifdef AMPDU_HAS_LEADING_FRAME
    if (p_ampdu_tx_data->leading_pkt != 0)
    	ENG_MBOX_SEND((tx_desc->txq_idx + M_ENG_TX_EDCA0), p_ampdu_tx_data->leading_pkt);
    #endif // AMPDU_HAS_LEADING_FRAME
    ENG_MBOX_SEND((tx_desc->txq_idx + M_ENG_TX_EDCA0), p_ampdu_tx_data->tx_pkt);
}


bool ampdu_tx_report_handler (u32 ampdu_tx_pkt)
{
    struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc*)ampdu_tx_pkt;

    #ifdef ENEABLE_MEAS_AMPDU_TX_TIME
    ampdu_tx_report_time = irq_time;
    #endif // ENEABLE_MEAS_AMPDU_TX_TIME

    int i, empty_entry = (-1), match_entry = (-1);
    int replace = 0;

    //printf("A - %d\n", tx_desc->rts_cts_nav);
    //prt_ssn("A", ampdu_tx_pkt);
    #ifdef AMPDU_HAS_LEADING_FRAME
    // New AMPDU frame to process?
    //  - Increase everytime the AMPDU frame is handled by MCU. So 0 is the frist time.
    if (tx_desc->RSVD_1++ == 0)//if (cur_cmd == M_ENG_CPU)
    {
    //	- Create null frame for RTS/CTS patch.
        u32 null_frame = _create_null_frame_for_ampdu(ampdu_tx_pkt);

    //	- Get empty TX data
        for (i = 0; i < MAX_CONCURRENT_AMPDU; i++)
            if (ampdu_tx_data[i].tx_pkt == 0)
            {
                empty_entry = i;
                tx_desc->TxF_ID = i;
                ampdu_tx_data[i].tx_pkt = ampdu_tx_pkt;
                ampdu_tx_data[i].leading_pkt = null_frame;
                ampdu_tx_data[i].retry_count = 0;
                break;
            }

    //  - Send to TX queue
        tx_desc->fCmdIdx++;
        if (null_frame != 0)
        {
            ((struct ssv6200_tx_desc *)null_frame)->TxF_ID = tx_desc->TxF_ID;
            ((struct ssv6200_tx_desc *)null_frame)->fCmdIdx++;
            ENG_MBOX_NEXT(null_frame);
        }
        ENG_MBOX_NEXT(ampdu_tx_pkt);

        if (i == MAX_CONCURRENT_AMPDU)
            prt_ssn("O", ampdu_tx_pkt);

        if (ampdu_st_reset)
            _reset_ampdu_mib();

        return true;
    }
    #endif // AMPDU_HAS_LEADING_FRAME

    // Check overflow of MCU process times tx_desc->RSVD_1.
    if (tx_desc->RSVD_1 == 0)
    	tx_desc->RSVD_1--;

    // Another AMPDU waiting BA? That means BA time-out.
    if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
    {
        us_timer_stop(US_TIMER0);
        //BUG_ON(ampdu_tx_data[current_waiting_BA_idx].tx_pkt == 0);
        //BUG_ON(ampdu_tx_data[current_waiting_BA_idx].tx_pkt == rx_data);

        // Exceeding maximum retry? Release it.
        if (ampdu_tx_data[current_waiting_BA_idx].retry_end == 1)
        {
            prt_ssn("CT", ampdu_tx_data[current_waiting_BA_idx].tx_pkt);
            ampdu_tx_data[current_waiting_BA_idx].tx_pkt =
                _notify_host_BA_drop(ampdu_tx_data[current_waiting_BA_idx].retry_count,
                                     ampdu_tx_data[current_waiting_BA_idx].tx_pkt, 
                                     ampdu_tx_data[current_waiting_BA_idx].tried_rates);
            ssv6200_ampdu_free_pkt_and_stop_timer(&ampdu_tx_data[current_waiting_BA_idx], US_TIMER0);
            ++ampdu_tx_retry_drop_count;
            ++ampdu_tx_drop_count;
        }
        // Later process retrying
        prt_ssn("N", ampdu_tx_pkt);
        if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
            prt_ssn("C", ampdu_tx_data[current_waiting_BA_idx].tx_pkt);
        replace = 1;
    }

    // Retry frame or new AMPDU frame?
    // Find the frame in current retry list. Find one empty for new AMPDU frame.
    if (   (tx_desc->TxF_ID < MAX_CONCURRENT_AMPDU)
    	&& (ampdu_tx_data[tx_desc->TxF_ID].tx_pkt == ampdu_tx_pkt))
    {
    	match_entry = tx_desc->TxF_ID;
    }
    else
    {
        for (i = 0;
    	     (   (i < MAX_CONCURRENT_AMPDU)
        	  && ((empty_entry == (-1)) || (match_entry == (-1))));
    	     i++)
    	{
    		if ((ampdu_tx_data[i].tx_pkt == 0) && empty_entry == (-1))
    		{
    			empty_entry = i;
    		}
    		else if (ampdu_tx_data[i].tx_pkt == ampdu_tx_pkt)
    		{
    			match_entry = i;
    		}
    	}
        // New AMPDU frame? Reset statistics for request
        if ((match_entry == (-1)) && ampdu_st_reset)
        {
        	_reset_ampdu_mib();
        }
    }

    ampdu_tx_count++;

    // New AMPDU frame but no empty entry for it.
    if ((empty_entry == (-1)) && (match_entry == (-1)))
    {
        ++ampdu_tx_force_drop_count;
        ++ampdu_tx_drop_count;
        // Force release current waiting one.
        if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
        {
            //WARN_ON(1);
            empty_entry = current_waiting_BA_idx;
            ampdu_tx_data[current_waiting_BA_idx].tx_pkt =
                _notify_host_BA_drop(ampdu_tx_data[current_waiting_BA_idx].retry_count, 
                                     ampdu_tx_data[current_waiting_BA_idx].tx_pkt, 
                                     ampdu_tx_data[current_waiting_BA_idx].tried_rates);
            ssv6200_ampdu_free_pkt_and_stop_timer(&ampdu_tx_data[current_waiting_BA_idx], US_TIMER0);
            prt_ssn("F", ampdu_tx_pkt);
        }
        else // New AMPDU and no entry for it? Just release it.
        {
            // ToDo: Use spare entry for such kind of situation because BA may come in later.
            struct ssv62xx_tx_rate tried_rates[SSV62XX_TX_MAX_RATES];

            //WARN_ON(1);
            ++ampdu_tx_1st_count;
            tried_rates[0].data_rate = (s8)(-1); // Not retry.
            prt_ssn("x", ampdu_tx_pkt);
            ampdu_tx_pkt = _notify_host_BA_drop(0, ampdu_tx_pkt, tried_rates);
            if (ampdu_tx_pkt)
                ENG_MBOX_SEND(M_ENG_TRASH_CAN, ampdu_tx_pkt);
            return true;
        }
    }
    // Current BA-waiting AMPDU can be retried.
    else if (current_waiting_BA_idx != MAX_CONCURRENT_AMPDU)
    {
        _retry_ampdu_frame(&ampdu_tx_data[current_waiting_BA_idx]);
    }

    // New AMPDU frame?
    if (match_entry == (-1))
    {
        match_entry = empty_entry;
        ampdu_tx_data[match_entry].tx_pkt = ampdu_tx_pkt;
        ampdu_tx_data[match_entry].retry_count = 0;
        tx_desc->TxF_ID = empty_entry;
        ++ampdu_tx_1st_count;
        ampdu_tx_data[match_entry].rates_idx = 0;
        ampdu_tx_data[match_entry].retry_end = 0;
        memset(ampdu_tx_data[match_entry].tried_rates, 0, sizeof(ampdu_tx_data[match_entry].tried_rates));
        ampdu_tx_data[match_entry].tried_rates[0].count = 1; //count for send times
        ampdu_tx_data[match_entry].tried_rates[0].data_rate = tx_desc->drate_idx;
        ampdu_tx_data[match_entry].tried_rates[1].data_rate = -1;
        ampdu_tx_data[match_entry].tried_rates[2].data_rate = -1;
        //prt_ssn("A", ampdu_tx_pkt);
    }
    // The second time an AMPDU TX frame got into MCU is the first TX report trap, i.e. the first time it is TXed.
    else if (tx_desc->RSVD_1 == 2)
    {
        ++ampdu_tx_1st_count;
    }
    else
    {
        ++ampdu_tx_retry_count;
    }

    current_waiting_BA_idx = match_entry;
    us_timer_start(US_TIMER0, AMPDU_FW_retry_time_us, match_entry, HTMR_ONESHOT);
    if (replace)
        PRINTF("R %d\n", current_waiting_BA_idx);

    return true;
}

static void _fill_seq_no (u32 retry_count, u32 tx_data, struct ampdu_ba_notify_data *dest, bool to_copy_header, u32 copy_header_size)
{
    struct ssv6200_tx_desc *tx_desc;
    AMPDU_DELIMITER *delim;
    struct ssv6200_80211_hdr *hdr;
    u32 mpdu_total_len;
    u32 acc_mpdu_len = 0;
    int i;

    delim = get_ampdu_delim_from_ssv_pkt(tx_data);
    hdr = get_80211hdr_from_delim(delim);

    tx_desc = (struct ssv6200_tx_desc*)tx_data;
    mpdu_total_len = tx_desc->len;

    dest->wsid = tx_desc->wsid;

    // Copy first frame header if asked to
    if (to_copy_header && copy_header_size)
    {
    	memcpy((void *)(dest + 1), hdr, copy_header_size);
    }

    // Fill up sequence #s
    for (i = 0; i < MAX_AGGR_NUM; i++)
    {
        u32 mpdu_len;

        dest->seq_no[i] = skb_ssn(hdr);

        mpdu_len = delim->length;
        mpdu_len += (4 - (mpdu_len % 4)) % 4;
        mpdu_len += SSV6200_AMPDU_DELIMITER_LENGTH;
        acc_mpdu_len += mpdu_len;

        if (acc_mpdu_len >= mpdu_total_len)
        {
            if (++i < MAX_AGGR_NUM)
                dest->seq_no[i] = (u16)(-1);
            break;
        }
        delim = get_next_delim(delim);
        hdr = get_80211hdr_from_delim(delim);
    }
}


u32 _notify_host_BA_drop (u32 retry_count, u32 tx_data, struct ssv62xx_tx_rate* record)
{
    u32 notification_data;
    HDR_HostEvent *host_evt;
    u32 copy_hdr_size = sizeof(struct ssv6200_80211_hdr) + 8;
    u32 evt_size =   sizeof(HDR_HostEvent) + sizeof(struct ampdu_ba_notify_data)
                   + copy_hdr_size;
    struct ampdu_ba_notify_data *notify_data = NULL;
    u32 ret = tx_data;

    // Allocate BA drop notification packet
    notification_data = (u32)PBUF_MAlloc_Raw(evt_size, 0, RX_BUF);
    if (notification_data == 0)
    {
        char buf[sizeof(struct ampdu_ba_notify_data) + sizeof(struct ssv6200_80211_hdr) + 8];
        prt_ssn("DD", tx_data);
        //return;
        // If allocation failed, re-use the TX frame for BA drop notification.
        // NOT use it ordinarily because reuse large AMPDU frame would make
        // page occupied longer. Lower TX throughput would be the consequence.

        // Fill up sequence #s to temporary buffer.
        _fill_seq_no(retry_count, tx_data, (struct ampdu_ba_notify_data *)buf, true, copy_hdr_size);
        notification_data = tx_data;
        host_evt = (HDR_HostEvent *)notification_data;
        memcpy(&host_evt->dat[0], buf, sizeof(buf));
        ret = 0;
    }
    else
    {
        host_evt = (HDR_HostEvent *)notification_data;
        // Fill up sequence #s
        _fill_seq_no(retry_count, tx_data, (struct ampdu_ba_notify_data *)&host_evt->dat[0], true, copy_hdr_size);
    }

    host_evt->c_type = HOST_EVENT;
    host_evt->h_event = SOC_EVT_NO_BA;
    host_evt->len = evt_size;
    notify_data = (struct ampdu_ba_notify_data *)&host_evt->dat[0];

    memcpy(&(notify_data->tried_rates), record, SSV62XX_TX_MAX_RATES*sizeof(struct ssv62xx_tx_rate));

    // Send out to host
    TX_FRAME(notification_data);

    return ret;
}

#define PKT_PAGE_SIZE    (256)

void _appand_ba_notify (u32 rx_data, u32 tx_data, u32 retry_count, struct ssv62xx_tx_rate* record)
{
    struct ssv6200_rx_desc_from_soc *rx_desc = (struct ssv6200_rx_desc_from_soc *)rx_data;
    u32 data_offset = GET_PB_OFFSET + rx_desc->len + 4; // Packet offset + packet len + phy info padding
    u32 page_dummy_size = PKT_PAGE_SIZE - (data_offset % PKT_PAGE_SIZE);
    struct ampdu_ba_notify_data *notify_data = (struct ampdu_ba_notify_data *)(rx_data + data_offset);

    if (page_dummy_size >= sizeof(struct ampdu_ba_notify_data))
    {
        //u32 orig_len = rx_desc->len;
        _fill_seq_no(retry_count, tx_data, notify_data, false, 0);
        memcpy(&(notify_data->tried_rates), record, SSV62XX_TX_MAX_RATES*sizeof(struct ssv62xx_tx_rate));
        rx_desc->len += sizeof(struct ampdu_ba_notify_data);
        //printf("B %d - %d - %d\n", orig_len, rx_desc->len, GET_PB_OFFSET);
    }
    else if (page_dummy_size >= sizeof(u16))
    {
        rx_desc->len += sizeof(u16);
        prt_ssn("n", tx_data);
    }
    else
    {
        prt_ssn("N", tx_data);
    }
}

#ifdef AMPDU_HAS_LEADING_FRAME 
u32 _create_leading_frame_for_ampdu (u32 tx_ampdu_pkt)
{
#if 1
    u32 			size = sizeof(HDR80211_Data);
    PKT_TxInfo 	   *tx_pkt;
	HDR80211_Data  *null_data_frame;
	u16 		   *fc;
	u32             retry_count = 10;
	ETHER_ADDR		bssid, sta_mac;
	//s32 			extra_size = 0;

	// Allocate packet buffer
    do {
    	tx_pkt = (PKT_TxInfo *)PBUF_MAlloc(size, TX_BUF);

    	if (tx_pkt != 0)
    		break;
    	if (retry_count--)
    		continue;
    	printf("nx\n");
    	return (u32)tx_pkt;
    } while (1);


	// Fill up TX descriptor
	//	- Copy from AM	PDU TX descriptor
    memcpy((void *)tx_pkt, (void *)tx_ampdu_pkt, size + GET_PB_OFFSET);
    //Fill_TxDescriptor(TxPkt, extra_size);
	//  - Modify for null frame specific fields.
    tx_pkt->len = HDR80211_MGMT_LEN;
    tx_pkt->hdr_len = HDR80211_MGMT_LEN;
    tx_pkt->do_rts_cts = 1;
    tx_pkt->rts_cts_nav = 1000;
    tx_pkt->aggregation = 0;
    tx_pkt->frame_consume_time = 8;
    tx_pkt->qos = 0;
    tx_pkt->ack_policy = 0;
    //tx_pkt->drate_idx = tx_pkt->crate_idx;

    null_data_frame = (HDR80211_Data *)((u8 *)tx_pkt + GET_PB_OFFSET);
    null_data_frame->dur = ((HDR80211_Data *)((u8 *)tx_ampdu_pkt + GET_PB_OFFSET))->dur;
    null_data_frame->seq_ctrl = 0;

    drv_mac_get_sta_mac(&sta_mac.addr[0]);
    drv_mac_get_bssid(&bssid.addr[0]);
#if 0
    printf("%02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X\n",
    		bssid.addr[0], bssid.addr[1], bssid.addr[2],
    		bssid.addr[3], bssid.addr[4], bssid.addr[5],
    		null_data_frame->addr1.addr[0], null_data_frame->addr1.addr[1], null_data_frame->addr1.addr[2],
    		null_data_frame->addr1.addr[3], null_data_frame->addr1.addr[4], null_data_frame->addr1.addr[5]);
#endif
    memcpy(&null_data_frame->addr1, bssid.addr, ETHER_ADDR_LEN);
    memcpy(&null_data_frame->addr2, sta_mac.addr, ETHER_ADDR_LEN);
    memcpy(&null_data_frame->addr3, bssid.addr, ETHER_ADDR_LEN);
//    printf("%d %d %d\n", GET_PB_OFFSET, tx_pkt->hdr_offset, tx_pkt->payload_offset);
    //memcpy(&null_data_frame->addr3, &null_data_frame->addr1, ETHER_ADDR_LEN);

    fc = (u16 *)&null_data_frame->fc;
    *fc = M_FC_TODS | FT_DATA | FST_NULLFUNC;

    return (u32)tx_pkt;
#else
    return 0;
#endif
}
#endif // AMPDU_HAS_LEADING_FRAME

void _reset_ampdu_mib (void)
{
    SSV6XXX_REG *phy_length_cnt_ctrl = (SSV6XXX_REG *)0xce0000a8;

    ampdu_st_reset = 0;
    ampdu_tx_count = 0;
    ampdu_tx_1st_count = 0;
    ampdu_tx_retry_count = 0;
    ampdu_tx_force_drop_count = 0;
    ampdu_tx_retry_drop_count = 0;
    ampdu_tx_drop_count = 0;
    ampdu_tx_ba_count = 0;
    ampdu_tx_1st_ba_count = 0;
    ampdu_tx_retry_ba_count = 0;
    *phy_length_cnt_ctrl = 0x00240020;
    *phy_length_cnt_ctrl = 0x80240020;

    #ifdef ENEABLE_MEAS_AMPDU_TX_TIME
    max_ba_elapsed_time = 0;
    total_ba_elapsed_time = 0;
    ba_elapsed_time_count = 0;
    small_timeout_count = 0;
    min_timeout_elapsed_time = (u32)(-1);
    #endif // ENEABLE_MEAS_AMPDU_TX_TIME
}


void ampdu_dump_retry_list(void)
{
    int i;
    
    PRINTF("Current waiting index: %d\n", current_waiting_BA_idx);
    
    for (i = 0; i < MAX_CONCURRENT_AMPDU; i++)
    {
        PRINTF("\t%d: %08X\n", i, ampdu_tx_data[i].tx_pkt);
        
        if (ampdu_tx_data[i].tx_pkt == 0)
        {
            continue;
        }
        else
        {
            AMPDU_DELIMITER *delim = get_ampdu_delim_from_ssv_pkt(ampdu_tx_data[i].tx_pkt);
            struct ssv6200_80211_hdr *hdr = get_80211hdr_from_delim(delim);
            
            PRINTF("\t\tSSN: %d\n"
                   "\t\tRetry: %d\n",
                   skb_ssn(hdr),
                   ampdu_tx_data[i].retry_count);
        }
    }
}
