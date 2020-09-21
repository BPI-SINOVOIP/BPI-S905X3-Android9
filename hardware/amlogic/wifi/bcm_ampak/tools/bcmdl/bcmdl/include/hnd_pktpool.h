/*
 * HND generic packet pool operation primitives
 *
 * Copyright (C) 2014, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: $
 */

#ifndef _hnd_pktpool_h_
#define _hnd_pktpool_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BCMPKTPOOL
#define POOL_ENAB(pool)		((pool) && (pool)->inited)
#define SHARED_POOL		(pktpool_shared)
#else 
#define POOL_ENAB(bus)		0
#define SHARED_POOL		((struct pktpool *)NULL)
#endif 

#ifdef BCMFRAGPOOL
#define SHARED_FRAG_POOL	(pktpool_shared_lfrag)
#endif
#define SHARED_RXFRAG_POOL	(pktpool_shared_rxlfrag)


#ifndef PKTPOOL_LEN_MAX
#define PKTPOOL_LEN_MAX		40
#endif 
#define PKTPOOL_CB_MAX		3


struct pktpool;

typedef void (*pktpool_cb_t)(struct pktpool *pool, void *arg);
typedef struct {
	pktpool_cb_t cb;
	void *arg;
} pktpool_cbinfo_t;

typedef int (*pktpool_cb_extn_t)(struct pktpool *pool, void *arg1, void* pkt, bool arg2);
typedef struct {
	pktpool_cb_extn_t cb;
	void *arg;
} pktpool_cbextn_info_t;


#ifdef BCMDBG_POOL

#define POOL_IDLE	0
#define POOL_RXFILL	1
#define POOL_RXDH	2
#define POOL_RXD11	3
#define POOL_TXDH	4
#define POOL_TXD11	5
#define POOL_AMPDU	6
#define POOL_TXENQ	7

typedef struct {
	void *p;
	uint32 cycles;
	uint32 dur;
} pktpool_dbg_t;

typedef struct {
	uint8 txdh;	
	uint8 txd11;	
	uint8 enq;	
	uint8 rxdh;	
	uint8 rxd11;	
	uint8 rxfill;	
	uint8 idle;	
} pktpool_stats_t;
#endif 

typedef struct pktpool {
	bool inited;            
	uint8 type;             
	uint8 id;               
	bool istx;              

	void * freelist;        
	uint16 avail;           
	uint16 len;             
	uint16 maxlen;          
	uint16 plen;            

	bool empty;
	uint8 cbtoggle;
	uint8 cbcnt;
	uint8 ecbcnt;
	bool emptycb_disable;
	pktpool_cbinfo_t *availcb_excl;
	pktpool_cbinfo_t cbs[PKTPOOL_CB_MAX];
	pktpool_cbinfo_t ecbs[PKTPOOL_CB_MAX];
	pktpool_cbextn_info_t cbext;
	pktpool_cbextn_info_t rxcplidfn;
#ifdef BCMDBG_POOL
	uint8 dbg_cbcnt;
	pktpool_cbinfo_t dbg_cbs[PKTPOOL_CB_MAX];
	uint16 dbg_qlen;
	pktpool_dbg_t dbg_q[PKTPOOL_LEN_MAX + 1];
#endif
	pktpool_cbinfo_t dmarxfill;
} pktpool_t;

extern pktpool_t *pktpool_shared;
#ifdef BCMFRAGPOOL
extern pktpool_t *pktpool_shared_lfrag;
#endif
extern pktpool_t *pktpool_shared_rxlfrag;


extern int pktpool_attach(osl_t *osh, uint32 total_pools);
extern int pktpool_dettach(osl_t *osh); 

extern int pktpool_init(osl_t *osh, pktpool_t *pktp, int *pktplen, int plen, bool istx, uint8 type);
extern int pktpool_deinit(osl_t *osh, pktpool_t *pktp);
extern int pktpool_fill(osl_t *osh, pktpool_t *pktp, bool minimal);
extern void* pktpool_get(pktpool_t *pktp);
extern void pktpool_free(pktpool_t *pktp, void *p);
extern int pktpool_add(pktpool_t *pktp, void *p);
extern int pktpool_avail_notify_normal(osl_t *osh, pktpool_t *pktp);
extern int pktpool_avail_notify_exclusive(osl_t *osh, pktpool_t *pktp, pktpool_cb_t cb);
extern int pktpool_avail_register(pktpool_t *pktp, pktpool_cb_t cb, void *arg);
extern int pktpool_empty_register(pktpool_t *pktp, pktpool_cb_t cb, void *arg);
extern int pktpool_setmaxlen(pktpool_t *pktp, uint16 maxlen);
extern int pktpool_setmaxlen_strict(osl_t *osh, pktpool_t *pktp, uint16 maxlen);
extern void pktpool_emptycb_disable(pktpool_t *pktp, bool disable);
extern bool pktpool_emptycb_disabled(pktpool_t *pktp);
extern int pktpool_hostaddr_fill_register(pktpool_t *pktp, pktpool_cb_extn_t cb, void *arg1);
extern int pktpool_rxcplid_fill_register(pktpool_t *pktp, pktpool_cb_extn_t cb, void *arg);
extern void pktpool_invoke_dmarxfill(pktpool_t *pktp);
extern int pkpool_haddr_avail_register_cb(pktpool_t *pktp, pktpool_cb_t cb, void *arg);

#define POOLPTR(pp)         ((pktpool_t *)(pp))
#define POOLID(pp)          (POOLPTR(pp)->id)

#define POOLSETID(pp, ppid) (POOLPTR(pp)->id = (ppid))

#define pktpool_len(pp)     (POOLPTR(pp)->len)
#define pktpool_avail(pp)   (POOLPTR(pp)->avail)
#define pktpool_plen(pp)    (POOLPTR(pp)->plen)
#define pktpool_maxlen(pp)  (POOLPTR(pp)->maxlen)



#define PKTPOOL_INVALID_ID          (0)
#define PKTPOOL_MAXIMUM_ID          (15)


extern pktpool_t *pktpools_registry[PKTPOOL_MAXIMUM_ID + 1];


#define PKTPOOL_ID2PTR(id)          (pktpools_registry[id])
#define PKTPOOL_PTR2ID(pp)          (POOLID(pp))


#ifdef BCMDBG_POOL
extern int pktpool_dbg_register(pktpool_t *pktp, pktpool_cb_t cb, void *arg);
extern int pktpool_start_trigger(pktpool_t *pktp, void *p);
extern int pktpool_dbg_dump(pktpool_t *pktp);
extern int pktpool_dbg_notify(pktpool_t *pktp);
extern int pktpool_stats_dump(pktpool_t *pktp, pktpool_stats_t *stats);
#endif 

#ifdef __cplusplus
	}
#endif

#endif 
