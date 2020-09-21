/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <asm/bitops.h>

#include "include/linux_usb_extra.h"
#include "include/linux_usb_com.h"
extern void pfree(void *ptr);
extern void *pmalloc(size_t size);

/*************************************************************
 * NAME:
 *      QueuePrepare
 *
 * DESCRIPTION:
*       Setup the queue function
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void QueuePrepare(csr_dev_t *dv)
{
    usb_queue_t *q;

    q = &(dv->queue);
    q->head = NULL;
    q->tail = NULL;
    atomic_set(&(q->count), 0);
    spin_lock_init(&(q->lock));
    init_waitqueue_head(&(q->wait));

    DBG_VERBOSE("Queue prepared\n");
}

/*************************************************************
 * NAME:
 *      QueueFree
 *
 * DESCRIPTION:
 *       Empty (free) all elements of the queue
 *
 * RETURNS:
 *      -
 *
 *************************************************************/
void QueueFree(csr_dev_t *dv)
{
    struct usb_qe *e;
    usb_queue_t *q;
    int i;
    int j;

    /* Get queue length */
    q = &(dv->queue);
    j = atomic_read(&(q->count));

    /* Remove all elements off the queue */
    i = 0;
    do
    {
        e = QueuePop(dv);

        /* If element existed, free buffer, message and element itself */
        if(e != NULL)
        {
            pfree(e->msg->buf);
            pfree(e->msg);
            pfree(e);
            i++;
        }
    }
    while(e != NULL);

    if(j != i)
    {
        printk(PRNPREFIX "Queue elements freed does not match queue count\n");
    }
    DBG_VERBOSE("Free queue: Contained %i, freed %i\n", j, i);

    atomic_set(&(q->count), 0);
}

/*************************************************************
 * NAME:
 *      QueueAppend
 *
 * DESCRIPTION:
 *      Append a new element to the tail of the queue.
 *
 * PARAMETERS:
 *      chan: BCSP channel id (see hc_com.h)
 *      data: Bluetooth data
 *      len: Length ofdata
 *      copy: Should we copy (true) data or consume it (false)
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void QueueAppend(csr_dev_t *dv, uint8_t chan, void *data,
    uint16_t len, bool copy)
{
    struct usb_qe *new;
    usb_queue_t *q;
    unsigned long irq;

    /* Alloc new element and BCSP structure */
    new = pmalloc(sizeof(struct usb_qe));
    new->chan = chan;
    new->next = NULL;

    new->msg = pmalloc(sizeof(MessageStructure));
    new->msg->buflen = len;

    /* Transfer / consume data */
    if(copy)
    {
        new->msg->buf = pmalloc(len);
        memcpy(new->msg->buf, data, len);
    }
    else
    {
        new->msg->buf = data;
    }

    q = &(dv->queue);
    spin_lock_irqsave(&(q->lock), irq);

    /* Installing head */
    if(q->head == NULL)
    {
        q->head = new;
    }

    /* Snap into the list */
    if(q->tail != NULL)
    {
        q->tail->next = new;
    }

    /* Always replace the tail */
    q->tail = new;
    atomic_inc(&(q->count));
    spin_unlock_irqrestore(&(q->lock), irq);

    DBG_VERBOSE("Appended element to queue, count is now %i\n",
                atomic_read(&(q->count)));
}


/*************************************************************
 * NAME:
 *      QueuePop
 *
 * DESCRIPTION:
 *       Pop queue head, ie. take off the first element
 *       The caller must free the returned element!
 *
 * RETURNS:
 *      -
 *
 *************************************************************/
struct usb_qe *QueuePop(csr_dev_t *dv)
{
    struct usb_qe *res;
    usb_queue_t *q;
    unsigned long irq;
    int i;

    res = NULL;
    q = &(dv->queue);
    i = atomic_read(&(q->count));

    DBG_VERBOSE("Popping element from queue (%i elements)\n", i);

    /* Bail out now if queue is empty */
    if(i == 0)
    {
        return NULL;
    }

    spin_lock_irqsave(&(q->lock), irq);

    /* Get head */
    if(q->head != NULL)
    {
        /* Chop off the head, and set to next in list */
        res = q->head;
        q->head = res->next;

        /* If head is also tail, update */
        if(res == q->tail)
        {
            q->tail = NULL;
        }
    }

    atomic_dec(&(q->count));
    spin_unlock_irqrestore(&(q->lock), irq);

    if(res != NULL)
    {
        DBG_VERBOSE("Popped element (chan=%d size=%d) from queue\n",
                    res->chan, res->msg->buflen);
    }

    return res;
}
