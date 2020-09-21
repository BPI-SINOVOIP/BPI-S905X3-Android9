/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SSV_DATA_STRUCT_H_
#define _SSV_DATA_STRUCT_H_ 
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/types.h>
struct ssv6xxx_queue {
    struct list_head queue;
    spinlock_t lock;
};
struct ssv6xxx_list_node {
    struct list_head list;
};
static inline void tu_ssv6xxx_init_queue(struct ssv6xxx_queue *ssv_queue) {
    INIT_LIST_HEAD(&ssv_queue->queue);
    spin_lock_init(&ssv_queue->lock);
}
static inline void tu_ssv6xxx_init_list_node(struct ssv6xxx_list_node *node) {
    INIT_LIST_HEAD(&node->list);
}
static inline void ssv6xxx_enqueue_list_node(struct ssv6xxx_list_node *node, struct ssv6xxx_queue *ssv_queue) {
    unsigned long flags;
    spin_lock_irqsave(&ssv_queue->lock, flags);
    list_del_init(&node->list);
    list_add_tail(&node->list, &ssv_queue->queue);
    spin_unlock_irqrestore(&ssv_queue->lock, flags);
}
static inline struct ssv6xxx_list_node *ssv6xxx_dequeue_list_node(struct ssv6xxx_queue *ssv_queue) {
    unsigned long flags;
    struct ssv6xxx_list_node *ssv_list_node;
    spin_lock_irqsave(&ssv_queue->lock, flags);
    if (list_empty(&ssv_queue->queue)) {
        ssv_list_node = NULL;
    } else {
        ssv_list_node = container_of((&ssv_queue->queue)->next, struct ssv6xxx_list_node, list);
        list_del_init(&ssv_list_node->list);
    }
    spin_unlock_irqrestore(&ssv_queue->lock, flags);
    return ssv_list_node;
}
#endif
