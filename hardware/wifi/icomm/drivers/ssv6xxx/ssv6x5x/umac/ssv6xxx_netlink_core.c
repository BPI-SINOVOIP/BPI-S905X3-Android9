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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <net/genetlink.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/pid_namespace.h>
#include <ssv6200_reg.h>
#include <ssv6200.h>
#include <hci/hctrl.h>
#include <smac/dev.h>
#include <smac/init.h>
#include "ssv6xxx_netlink_core.h"
static char *attach_driver_name = "TU SSV WLAN driver";
module_param(attach_driver_name, charp, S_IRUGO|S_IWUSR);
static char *attach_device_name = "SSV6XXX";
module_param(attach_device_name, charp, S_IRUGO|S_IWUSR);
#define MAX_SSV_WIRELESS_PATTERN 32767
static struct nla_policy ssv_wireless_genl_policy[SSV_WIRELESS_ATTR_MAX + 1] = {
 [SSV_WIRELESS_ATTR_CONFIG] = { .type = NLA_U32 },
 [SSV_WIRELESS_ATTR_REGISTER] = { .len = sizeof(struct ssv_wireless_register) },
 [SSV_WIRELESS_ATTR_TXFRAME] = { .type = NLA_BINARY, .len = MAX_SSV_WIRELESS_PATTERN },
 [SSV_WIRELESS_ATTR_RXFRAME] = { .type = NLA_BINARY, .len = MAX_SSV_WIRELESS_PATTERN },
 [SSV_WIRELESS_ATTR_START_HCI] = { .type = NLA_U32 },
 [SSV_WIRELESS_ATTR_TEST] = { .type = NLA_BINARY, .len = MAX_SSV_WIRELESS_PATTERN },
};
static struct genl_family ssv_wireless_gnl_family = {
 .id = GENL_ID_GENERATE,
 .hdrsize = 0,
 .name = "SSV_WIRELESS",
 .version = 1,
 .maxattr = SSV_WIRELESS_ATTR_MAX,
};
static u32 ssv6xxx_user_app_pid = 0;
int ssv_wireless_config(struct sk_buff *skb, struct genl_info *info)
{
 if (info == NULL)
  return -EINVAL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
 ssv6xxx_user_app_pid = info->snd_portid;
#else
 ssv6xxx_user_app_pid = info->snd_pid;
#endif
 printk("config ssv6xxx user application pid[%d]\n", ssv6xxx_user_app_pid);
 return 0;
}
int ssv_wireless_start_hci(struct sk_buff *skb, struct genl_info *info)
{
 if (info == NULL)
  return -EINVAL;
 printk("start HCI\n");
 ssv6xxx_umac_hci_start(attach_driver_name, attach_device_name);
 return 0;
}
int ssv_wireless_test(struct sk_buff *skb, struct genl_info *info)
{
    struct nlattr *na;
    int sdio_dma_data_len;
    char *data;
    struct sk_buff *sdio_dma_frame;
    if (info == NULL)
        return -EINVAL;
    na = info->attrs[SSV_WIRELESS_ATTR_TEST];
    if (na) {
        if ((data = (char *)nla_data(na)) == NULL)
            return -EINVAL;
        sdio_dma_data_len = na->nla_len - NLA_HDRLEN;
    }
    else
        return -EINVAL;
    sdio_dma_frame = alloc_skb(sdio_dma_data_len, GFP_ATOMIC);
    if (sdio_dma_frame == NULL)
        return -EINVAL;
    skb_put(sdio_dma_frame, sdio_dma_data_len);
    memcpy(sdio_dma_frame->data, data, sdio_dma_data_len);
    ssv6xxx_umac_test(attach_driver_name, attach_device_name, sdio_dma_frame, sdio_dma_data_len);
    kfree_skb(sdio_dma_frame);
    return 0;
}
int ssv_wireless_read_register(struct sk_buff *skb, struct genl_info *info)
{
 struct nlattr *na;
 struct sk_buff *msg;
 int retval;
 void *msg_head;
 struct ssv_wireless_register *reg;
 if (info == NULL)
  return -EINVAL;
 na = info->attrs[SSV_WIRELESS_ATTR_REGISTER];
 if (na) {
  if ((reg = (struct ssv_wireless_register *)nla_data(na)) == NULL)
   return -EINVAL;
 }
 else
  return -EINVAL;
 ssv6xxx_umac_reg_read(attach_driver_name, attach_device_name, reg->address, &reg->value);
 msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
 if (msg == NULL)
  return -ENOMEM;
 msg_head = genlmsg_put(msg, 0, info->snd_seq+1, &ssv_wireless_gnl_family, 0, SSV_WIRELESS_CMD_READ_REG);
 if (msg_head == NULL) {
  retval = -ENOMEM;
  printk("Fail to create the netlink message header\n");
  goto free_msg;
 }
 retval = nla_put(msg, SSV_WIRELESS_ATTR_REGISTER, sizeof(struct ssv_wireless_register), reg);
 if (retval != 0) {
  printk("Fail to add attribute in message\n");
  goto free_msg;
 }
 genlmsg_end(msg, msg_head);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
 return genlmsg_unicast(genl_info_net(info), msg, info->snd_portid);
#else
 return genlmsg_unicast(genl_info_net(info), msg, info->snd_pid);
#endif
free_msg:
 nlmsg_free(msg);
 return retval;
}
int ssv_wireless_write_register(struct sk_buff *skb, struct genl_info *info)
{
 struct nlattr *na;
 struct sk_buff *msg;
 int retval;
 void *msg_head;
 struct ssv_wireless_register *reg;
 if (info == NULL)
  return -EINVAL;
 na = info->attrs[SSV_WIRELESS_ATTR_REGISTER];
 if (na) {
  if ((reg = (struct ssv_wireless_register *)nla_data(na)) == NULL)
   return -EINVAL;
 }
 else
  return -EINVAL;
 ssv6xxx_umac_reg_write(attach_driver_name, attach_device_name, reg->address, reg->value);
 msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
 if (msg == NULL)
  return -ENOMEM;
 msg_head = genlmsg_put(msg, 0, info->snd_seq+1, &ssv_wireless_gnl_family, 0, SSV_WIRELESS_CMD_WRITE_REG);
 if (msg_head == NULL) {
  retval = -ENOMEM;
  printk("Fail to create the netlink message header\n");
  goto free_msg;
 }
 retval = nla_put(msg, SSV_WIRELESS_ATTR_REGISTER, sizeof(struct ssv_wireless_register), reg);
 if (retval != 0) {
  printk("Fail to add attribute in message\n");
  goto free_msg;
 }
 genlmsg_end(msg, msg_head);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
 return genlmsg_unicast(genl_info_net(info), msg, info->snd_portid);
#else
 return genlmsg_unicast(genl_info_net(info), msg, info->snd_pid);
#endif
free_msg:
 nlmsg_free(msg);
 return retval;
}
int ssv_wireless_tx_frame(struct sk_buff *skb, struct genl_info *info)
{
 struct nlattr *na;
 int tx_data_len;
 char *data;
 struct sk_buff *tx_frame;
 if (info == NULL)
  return -EINVAL;
 na = info->attrs[SSV_WIRELESS_ATTR_TXFRAME];
 if (na) {
  if ((data = (char *)nla_data(na)) == NULL)
   return -EINVAL;
  tx_data_len = na->nla_len - NLA_HDRLEN;
        printk("tx_data_len=%x\n",tx_data_len);
 }
 else
  return -EINVAL;
 tx_frame = alloc_skb(tx_data_len, GFP_ATOMIC);
 if (tx_frame == NULL)
  return -EINVAL;
 skb_put(tx_frame, tx_data_len);
 memcpy(tx_frame->data, data, tx_data_len);
 ssv6xxx_umac_tx_frame(attach_driver_name, attach_device_name, tx_frame);
 kfree_skb(tx_frame);
 return 0;
}
static int ssv_wireless_umac_rx_raw(struct sk_buff *skb)
{
 struct sk_buff *msg;
 int retval;
 void *msg_head;
 struct pid *pid_struct;
 struct task_struct *task;
 u32 pid = ssv6xxx_user_app_pid;
 if (pid == 0) {
  printk("The destination is not ready!");
  return 0;
 }
 pid_struct = find_get_pid(pid);
 if ((task = pid_task(pid_struct, PIDTYPE_PID)) == NULL) {
  printk("Userspace appllication is not existed!");
  return 0;
 }
 msg = genlmsg_new(MAX_SSV_WIRELESS_PATTERN, GFP_KERNEL);
 if (msg == NULL)
  return -ENOMEM;
 msg_head = genlmsg_put(msg, 0, 0, &ssv_wireless_gnl_family, 0, SSV_WIRELESS_CMD_RXFRAME);
 if (msg_head == NULL) {
  retval = -ENOMEM;
  printk("Fail to create the netlink message header\n");
  goto free_msg;
 }
 retval = nla_put(msg, SSV_WIRELESS_ATTR_RXFRAME, skb->len, skb->data);
 if (retval != 0) {
  printk("Fail to add attribute in message\n");
  goto free_msg;
 }
 genlmsg_end(msg, msg_head);
 return genlmsg_unicast(&init_net, msg, pid);
free_msg:
 nlmsg_free(msg);
 return retval;
}
struct ssv6xxx_umac_ops ssv_umac = {
 .umac_rx_raw = ssv_wireless_umac_rx_raw,
};
struct genl_ops ssv_wireless_gnl_ops[] = {
 {
  .cmd = SSV_WIRELESS_CMD_CONFIG,
  .flags = 0,
  .policy = ssv_wireless_genl_policy,
  .doit = ssv_wireless_config,
  .dumpit = NULL,
 },
 {
  .cmd = SSV_WIRELESS_CMD_READ_REG,
  .flags = 0,
  .policy = ssv_wireless_genl_policy,
  .doit = ssv_wireless_read_register,
  .dumpit = NULL,
 },
 {
  .cmd = SSV_WIRELESS_CMD_WRITE_REG,
  .flags = 0,
  .policy = ssv_wireless_genl_policy,
  .doit = ssv_wireless_write_register,
  .dumpit = NULL,
 },
 {
  .cmd = SSV_WIRELESS_CMD_TXFRAME,
  .flags = 0,
  .policy = ssv_wireless_genl_policy,
  .doit = ssv_wireless_tx_frame,
  .dumpit = NULL,
 },
 {
  .cmd = SSV_WIRELESS_CMD_START_HCI,
  .flags = 0,
  .policy = ssv_wireless_genl_policy,
  .doit = ssv_wireless_start_hci,
  .dumpit = NULL,
 },
    {
        .cmd = SSV_WIRELESS_CMD_TEST,
        .flags = 0,
        .policy = ssv_wireless_genl_policy,
        .doit = ssv_wireless_test,
        .dumpit = NULL,
    },
};
static int __init ssv_netlink_init(void)
{
 int rc;
 printk("INIT SSV WIRELESS GENERIC NETLINK MODULE\n");
 rc = ssv6xxx_umac_attach(attach_driver_name, attach_device_name, &ssv_umac);
 if (rc != 0)
  return -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
 rc = genl_register_family_with_ops(&ssv_wireless_gnl_family,
  ssv_wireless_gnl_ops);
#else
 rc = genl_register_family_with_ops(&ssv_wireless_gnl_family,
  ssv_wireless_gnl_ops, ARRAY_SIZE(ssv_wireless_gnl_ops));
#endif
    if (rc != 0) {
  printk("Fail to insert SSV WIRELESS NETLINK MODULE\n");
  return -1;
 }
 return 0;
}
static void __exit ssv_netlink_exit(void)
{
 int rc;
 printk("EXIT SSV WIRELESS GENERIC NETLINK MODULE\n");
 rc = genl_unregister_family(&ssv_wireless_gnl_family);
 if (rc != 0)
  printk("unregister family %d\n", rc);
 rc = ssv6xxx_umac_deattach(attach_driver_name, attach_device_name);
 if (rc != 0)
  printk("Fail to deattach ssv wlan umac\n");
}
module_init(ssv_netlink_init);
module_exit(ssv_netlink_exit);
MODULE_AUTHOR("iComm-semi, Ltd");
MODULE_DESCRIPTION("Support for SSV6xxx wireless LAN cards.");
MODULE_LICENSE("Dual BSD/GPL");
