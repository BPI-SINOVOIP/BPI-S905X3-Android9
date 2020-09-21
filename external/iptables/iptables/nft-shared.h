#ifndef _NFT_SHARED_H_
#define _NFT_SHARED_H_

#include <stdbool.h>

#include <libnftnl/rule.h>
#include <libnftnl/expr.h>

#include <linux/netfilter_arp/arp_tables.h>

#include "xshared.h"

#if 0
#define DEBUGP(x, args...) fprintf(stdout, x, ## args)
#define NLDEBUG
#define DEBUG_DEL
#else
#define DEBUGP(x, args...)
#endif

/*
 * iptables print output emulation
 */

#define FMT_NUMERIC	0x0001
#define FMT_NOCOUNTS	0x0002
#define FMT_KILOMEGAGIGA 0x0004
#define FMT_OPTIONS	0x0008
#define FMT_NOTABLE	0x0010
#define FMT_NOTARGET	0x0020
#define FMT_VIA		0x0040
#define FMT_NONEWLINE	0x0080
#define FMT_LINENUMBERS 0x0100

#define FMT_PRINT_RULE (FMT_NOCOUNTS | FMT_OPTIONS | FMT_VIA \
			| FMT_NUMERIC | FMT_NOTABLE)
#define FMT(tab,notab) ((format) & FMT_NOTABLE ? (notab) : (tab))

struct xtables_args;
struct xt_xlate;

enum {
	NFT_XT_CTX_PAYLOAD	= (1 << 0),
	NFT_XT_CTX_META		= (1 << 1),
	NFT_XT_CTX_BITWISE	= (1 << 2),
};

struct nft_xt_ctx {
	union {
		struct iptables_command_state *cs;
		struct arptables_command_state *cs_arp;
		struct ebtables_command_state *cs_eb;
	} state;
	struct nftnl_expr_iter *iter;
	int family;
	uint32_t flags;

	uint32_t reg;
	struct {
		uint32_t offset;
		uint32_t len;
	} payload;
	struct {
		uint32_t key;
	} meta;
	struct {
		uint32_t mask[4];
		uint32_t xor[4];
	} bitwise;
};

struct nft_family_ops {
	int (*add)(struct nftnl_rule *r, void *data);
	bool (*is_same)(const void *data_a,
			const void *data_b);
	void (*print_payload)(struct nftnl_expr *e,
			      struct nftnl_expr_iter *iter);
	void (*parse_meta)(struct nft_xt_ctx *ctx, struct nftnl_expr *e,
			   void *data);
	void (*parse_payload)(struct nft_xt_ctx *ctx, struct nftnl_expr *e,
			      void *data);
	void (*parse_bitwise)(struct nft_xt_ctx *ctx, struct nftnl_expr *e,
			      void *data);
	void (*parse_cmp)(struct nft_xt_ctx *ctx, struct nftnl_expr *e,
			  void *data);
	void (*parse_immediate)(const char *jumpto, bool nft_goto, void *data);

	void (*print_table_header)(const char *tablename);
	void (*print_header)(unsigned int format, const char *chain,
			     const char *pol,
			     const struct xt_counters *counters, bool basechain,
			     uint32_t refs);
	void (*print_firewall)(struct nftnl_rule *r, unsigned int num,
			       unsigned int format);
	void (*save_firewall)(const void *data, unsigned int format);
	void (*save_counters)(const void *data);
	void (*proto_parse)(struct iptables_command_state *cs,
			    struct xtables_args *args);
	void (*post_parse)(int command, struct iptables_command_state *cs,
			   struct xtables_args *args);
	void (*parse_match)(struct xtables_match *m, void *data);
	void (*parse_target)(struct xtables_target *t, void *data);
	bool (*rule_find)(struct nft_family_ops *ops, struct nftnl_rule *r,
			  void *data);
	int (*xlate)(const void *data, struct xt_xlate *xl);
};

void add_meta(struct nftnl_rule *r, uint32_t key);
void add_payload(struct nftnl_rule *r, int offset, int len, uint32_t base);
void add_bitwise_u16(struct nftnl_rule *r, int mask, int xor);
void add_cmp_ptr(struct nftnl_rule *r, uint32_t op, void *data, size_t len);
void add_cmp_u8(struct nftnl_rule *r, uint8_t val, uint32_t op);
void add_cmp_u16(struct nftnl_rule *r, uint16_t val, uint32_t op);
void add_cmp_u32(struct nftnl_rule *r, uint32_t val, uint32_t op);
void add_iniface(struct nftnl_rule *r, char *iface, uint32_t op);
void add_outiface(struct nftnl_rule *r, char *iface, uint32_t op);
void add_addr(struct nftnl_rule *r, int offset,
	      void *data, void *mask, size_t len, uint32_t op);
void add_proto(struct nftnl_rule *r, int offset, size_t len,
	       uint8_t proto, uint32_t op);
void add_compat(struct nftnl_rule *r, uint32_t proto, bool inv);

bool is_same_interfaces(const char *a_iniface, const char *a_outiface,
			unsigned const char *a_iniface_mask,
			unsigned const char *a_outiface_mask,
			const char *b_iniface, const char *b_outiface,
			unsigned const char *b_iniface_mask,
			unsigned const char *b_outiface_mask);

int parse_meta(struct nftnl_expr *e, uint8_t key, char *iniface,
		unsigned char *iniface_mask, char *outiface,
		unsigned char *outiface_mask, uint8_t *invflags);
void print_proto(uint16_t proto, int invert);
void get_cmp_data(struct nftnl_expr *e, void *data, size_t dlen, bool *inv);
void nft_parse_bitwise(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_cmp(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_match(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_target(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_meta(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_payload(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_parse_counter(struct nftnl_expr *e, struct xt_counters *counters);
void nft_parse_immediate(struct nft_xt_ctx *ctx, struct nftnl_expr *e);
void nft_rule_to_iptables_command_state(struct nftnl_rule *r,
					struct iptables_command_state *cs);
void print_header(unsigned int format, const char *chain, const char *pol,
		  const struct xt_counters *counters, bool basechain,
		  uint32_t refs);
void print_firewall_details(const struct iptables_command_state *cs,
			    const char *targname, uint8_t flags,
			    uint8_t invflags, uint8_t proto,
			    unsigned int num, unsigned int format);
void print_ifaces(const char *iniface, const char *outiface, uint8_t invflags,
		  unsigned int format);
void print_matches_and_target(struct iptables_command_state *cs,
			      unsigned int format);
void save_firewall_details(const struct iptables_command_state *cs,
			   uint8_t invflags, uint16_t proto,
			   const char *iniface,
			   unsigned const char *iniface_mask,
			   const char *outiface,
			   unsigned const char *outiface_mask);
void save_counters(uint64_t pcnt, uint64_t bcnt);
void save_matches_and_target(struct xtables_rule_match *m,
			     struct xtables_target *target,
			     const char *jumpto,
			     uint8_t flags, const void *fw);

struct nft_family_ops *nft_family_ops_lookup(int family);

struct nft_handle;
bool nft_ipv46_rule_find(struct nft_family_ops *ops, struct nftnl_rule *r,
			 struct iptables_command_state *cs);

bool compare_matches(struct xtables_rule_match *mt1, struct xtables_rule_match *mt2);
bool compare_targets(struct xtables_target *tg1, struct xtables_target *tg2);

struct addr_mask {
	union {
		struct in_addr	*v4;
		struct in6_addr *v6;
	} addr;

	unsigned int naddrs;

	union {
		struct in_addr	*v4;
		struct in6_addr *v6;
	} mask;
};

struct xtables_args {
	int		family;
	uint16_t	proto;
	uint8_t		flags;
	uint8_t		invflags;
	char		iniface[IFNAMSIZ], outiface[IFNAMSIZ];
	unsigned char	iniface_mask[IFNAMSIZ], outiface_mask[IFNAMSIZ];
	bool		goto_set;
	const char	*shostnetworkmask, *dhostnetworkmask;
	const char	*pcnt, *bcnt;
	struct addr_mask s, d;
	unsigned long long pcnt_cnt, bcnt_cnt;
};

#define CMD_NONE		0x0000U
#define CMD_INSERT		0x0001U
#define CMD_DELETE		0x0002U
#define CMD_DELETE_NUM		0x0004U
#define CMD_REPLACE		0x0008U
#define CMD_APPEND		0x0010U
#define CMD_LIST		0x0020U
#define CMD_FLUSH		0x0040U
#define CMD_ZERO		0x0080U
#define CMD_NEW_CHAIN		0x0100U
#define CMD_DELETE_CHAIN	0x0200U
#define CMD_SET_POLICY		0x0400U
#define CMD_RENAME_CHAIN	0x0800U
#define CMD_LIST_RULES		0x1000U
#define CMD_ZERO_NUM		0x2000U
#define CMD_CHECK		0x4000U

struct nft_xt_cmd_parse {
	unsigned int			command;
	unsigned int			rulenum;
	char				*table;
	char				*chain;
	char				*newname;
	char				*policy;
	bool				restore;
	int				verbose;
};

void do_parse(struct nft_handle *h, int argc, char *argv[],
	      struct nft_xt_cmd_parse *p, struct iptables_command_state *cs,
	      struct xtables_args *args);

struct nft_xt_restore_parse {
	FILE		*in;
	int		testing;
	const char	*tablename;
};

struct nftnl_chain_list;

struct nft_xt_restore_cb {
	void (*table_new)(struct nft_handle *h, const char *table);
	struct nftnl_chain_list *(*chain_list)(struct nft_handle *h);
	int (*chains_purge)(struct nft_handle *h, const char *table,
			    struct nftnl_chain_list *clist);
	void (*chain_del)(struct nftnl_chain_list *clist, const char *curtable,
			  const char *chain);
	int (*chain_set)(struct nft_handle *h, const char *table,
			 const char *chain, const char *policy,
			 const struct xt_counters *counters);
	int (*chain_user_add)(struct nft_handle *h, const char *chain,
			      const char *table);

	int (*rule_flush)(struct nft_handle *h, const char *chain, const char *table);

	int (*do_command)(struct nft_handle *h, int argc, char *argv[],
			  char **table, bool restore);

	int (*commit)(struct nft_handle *h);
	int (*abort)(struct nft_handle *h);
};

void xtables_restore_parse(struct nft_handle *h,
			   struct nft_xt_restore_parse *p,
			   struct nft_xt_restore_cb *cb,
			   int argc, char *argv[]);

#endif
