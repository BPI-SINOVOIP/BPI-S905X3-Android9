/*
 * (C) 2012-2014 by Pablo Neira Ayuso <pablo@netfilter.org>
 * (C) 2013 by Tomasz Bursztyka <tomasz.bursztyka@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This code has been sponsored by Sophos Astaro <http://www.sophos.com>
 */

#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <netdb.h>

#include <xtables.h>

#include <linux/netfilter/nf_tables.h>
#include "nft.h"
#include "nft-shared.h"

static int nft_ipv6_add(struct nftnl_rule *r, void *data)
{
	struct iptables_command_state *cs = data;
	struct xtables_rule_match *matchp;
	uint32_t op;
	int ret;

	if (cs->fw6.ipv6.iniface[0] != '\0') {
		op = nft_invflags2cmp(cs->fw6.ipv6.invflags, IPT_INV_VIA_IN);
		add_iniface(r, cs->fw6.ipv6.iniface, op);
	}

	if (cs->fw6.ipv6.outiface[0] != '\0') {
		op = nft_invflags2cmp(cs->fw6.ipv6.invflags, IPT_INV_VIA_OUT);
		add_outiface(r, cs->fw6.ipv6.outiface, op);
	}

	if (cs->fw6.ipv6.proto != 0) {
		op = nft_invflags2cmp(cs->fw6.ipv6.invflags, XT_INV_PROTO);
		add_proto(r, offsetof(struct ip6_hdr, ip6_nxt), 1,
			  cs->fw6.ipv6.proto, op);
	}

	if (!IN6_IS_ADDR_UNSPECIFIED(&cs->fw6.ipv6.src)) {
		op = nft_invflags2cmp(cs->fw6.ipv6.invflags, IPT_INV_SRCIP);
		add_addr(r, offsetof(struct ip6_hdr, ip6_src),
			 &cs->fw6.ipv6.src, &cs->fw6.ipv6.smsk,
			 sizeof(struct in6_addr), op);
	}
	if (!IN6_IS_ADDR_UNSPECIFIED(&cs->fw6.ipv6.dst)) {
		op = nft_invflags2cmp(cs->fw6.ipv6.invflags, IPT_INV_DSTIP);
		add_addr(r, offsetof(struct ip6_hdr, ip6_dst),
			 &cs->fw6.ipv6.dst, &cs->fw6.ipv6.dmsk,
			 sizeof(struct in6_addr), op);
	}
	add_compat(r, cs->fw6.ipv6.proto, cs->fw6.ipv6.invflags);

	for (matchp = cs->matches; matchp; matchp = matchp->next) {
		/* Use nft built-in comments support instead of comment match */
		if (strcmp(matchp->match->name, "comment") == 0) {
			ret = add_comment(r, (char *)matchp->match->m->data);
			if (ret < 0)
				return ret;
		} else {
			ret = add_match(r, matchp->match->m);
			if (ret < 0)
				return ret;
		}
	}

	/* Counters need to me added before the target, otherwise they are
	 * increased for each rule because of the way nf_tables works.
	 */
	if (add_counters(r, cs->counters.pcnt, cs->counters.bcnt) < 0)
		return -1;

	return add_action(r, cs, !!(cs->fw6.ipv6.flags & IP6T_F_GOTO));
}

static bool nft_ipv6_is_same(const void *data_a,
			     const void *data_b)
{
	const struct iptables_command_state *a = data_a;
	const struct iptables_command_state *b = data_b;

	if (memcmp(a->fw6.ipv6.src.s6_addr, b->fw6.ipv6.src.s6_addr,
		   sizeof(struct in6_addr)) != 0
	    || memcmp(a->fw6.ipv6.dst.s6_addr, b->fw6.ipv6.dst.s6_addr,
		    sizeof(struct in6_addr)) != 0
	    || a->fw6.ipv6.proto != b->fw6.ipv6.proto
	    || a->fw6.ipv6.flags != b->fw6.ipv6.flags
	    || a->fw6.ipv6.invflags != b->fw6.ipv6.invflags) {
		DEBUGP("different src/dst/proto/flags/invflags\n");
		return false;
	}

	return is_same_interfaces(a->fw6.ipv6.iniface, a->fw6.ipv6.outiface,
				  a->fw6.ipv6.iniface_mask,
				  a->fw6.ipv6.outiface_mask,
				  b->fw6.ipv6.iniface, b->fw6.ipv6.outiface,
				  b->fw6.ipv6.iniface_mask,
				  b->fw6.ipv6.outiface_mask);
}

static void nft_ipv6_parse_meta(struct nft_xt_ctx *ctx, struct nftnl_expr *e,
				void *data)
{
	struct iptables_command_state *cs = data;

	parse_meta(e, ctx->meta.key, cs->fw6.ipv6.iniface,
		   cs->fw6.ipv6.iniface_mask, cs->fw6.ipv6.outiface,
		   cs->fw6.ipv6.outiface_mask, &cs->fw6.ipv6.invflags);
}

static void parse_mask_ipv6(struct nft_xt_ctx *ctx, struct in6_addr *mask)
{
	memcpy(mask, ctx->bitwise.mask, sizeof(struct in6_addr));
}

static void nft_ipv6_parse_payload(struct nft_xt_ctx *ctx,
				   struct nftnl_expr *e, void *data)
{
	struct iptables_command_state *cs = data;
	struct in6_addr addr;
	uint8_t proto;
	bool inv;

	switch (ctx->payload.offset) {
	case offsetof(struct ip6_hdr, ip6_src):
		get_cmp_data(e, &addr, sizeof(addr), &inv);
		memcpy(cs->fw6.ipv6.src.s6_addr, &addr, sizeof(addr));
		if (ctx->flags & NFT_XT_CTX_BITWISE) {
			parse_mask_ipv6(ctx, &cs->fw6.ipv6.smsk);
			ctx->flags &= ~NFT_XT_CTX_BITWISE;
		} else {
			memset(&cs->fw.ip.smsk, 0xff, sizeof(struct in6_addr));
		}

		if (inv)
			cs->fw6.ipv6.invflags |= IP6T_INV_SRCIP;
		break;
	case offsetof(struct ip6_hdr, ip6_dst):
		get_cmp_data(e, &addr, sizeof(addr), &inv);
		memcpy(cs->fw6.ipv6.dst.s6_addr, &addr, sizeof(addr));
		if (ctx->flags & NFT_XT_CTX_BITWISE) {
			parse_mask_ipv6(ctx, &cs->fw6.ipv6.dmsk);
			ctx->flags &= ~NFT_XT_CTX_BITWISE;
		} else {
			memset(&cs->fw.ip.dmsk, 0xff, sizeof(struct in6_addr));
		}

		if (inv)
			cs->fw6.ipv6.invflags |= IP6T_INV_DSTIP;
		break;
	case offsetof(struct ip6_hdr, ip6_nxt):
		get_cmp_data(e, &proto, sizeof(proto), &inv);
		cs->fw6.ipv6.flags |= IP6T_F_PROTO;
		cs->fw6.ipv6.proto = proto;
		if (inv)
			cs->fw6.ipv6.invflags |= IP6T_INV_PROTO;
	default:
		DEBUGP("unknown payload offset %d\n", ctx->payload.offset);
		break;
	}
}

static void nft_ipv6_parse_immediate(const char *jumpto, bool nft_goto,
				     void *data)
{
	struct iptables_command_state *cs = data;

	cs->jumpto = jumpto;

	if (nft_goto)
		cs->fw6.ipv6.flags |= IP6T_F_GOTO;
}

static void nft_ipv6_print_header(unsigned int format, const char *chain,
				  const char *pol,
				  const struct xt_counters *counters,
				  bool basechain, uint32_t refs)
{
	print_header(format, chain, pol, counters, basechain, refs);
}

static void print_ipv6_addr(const struct iptables_command_state *cs,
			    unsigned int format)
{
	char buf[BUFSIZ];

	fputc(cs->fw6.ipv6.invflags & IP6T_INV_SRCIP ? '!' : ' ', stdout);
	if (IN6_IS_ADDR_UNSPECIFIED(&cs->fw6.ipv6.src)
	    && !(format & FMT_NUMERIC))
		printf(FMT("%-19s ","%s "), "anywhere");
	else {
		if (format & FMT_NUMERIC)
			strcpy(buf,
			       xtables_ip6addr_to_numeric(&cs->fw6.ipv6.src));
		else
			strcpy(buf,
			       xtables_ip6addr_to_anyname(&cs->fw6.ipv6.src));
		strcat(buf, xtables_ip6mask_to_numeric(&cs->fw6.ipv6.smsk));
		printf(FMT("%-19s ","%s "), buf);
	}


	fputc(cs->fw6.ipv6.invflags & IP6T_INV_DSTIP ? '!' : ' ', stdout);
	if (IN6_IS_ADDR_UNSPECIFIED(&cs->fw6.ipv6.dst)
	    && !(format & FMT_NUMERIC))
		printf(FMT("%-19s ","-> %s"), "anywhere");
	else {
		if (format & FMT_NUMERIC)
			strcpy(buf,
			       xtables_ip6addr_to_numeric(&cs->fw6.ipv6.dst));
		else
			strcpy(buf,
			       xtables_ip6addr_to_anyname(&cs->fw6.ipv6.dst));
		strcat(buf, xtables_ip6mask_to_numeric(&cs->fw6.ipv6.dmsk));
		printf(FMT("%-19s ","-> %s"), buf);
	}
}

static void nft_ipv6_print_firewall(struct nftnl_rule *r, unsigned int num,
				    unsigned int format)
{
	struct iptables_command_state cs = {};

	nft_rule_to_iptables_command_state(r, &cs);

	print_firewall_details(&cs, cs.jumpto, cs.fw6.ipv6.flags,
			       cs.fw6.ipv6.invflags, cs.fw6.ipv6.proto,
			       num, format);
	print_ifaces(cs.fw6.ipv6.iniface, cs.fw6.ipv6.outiface,
		     cs.fw6.ipv6.invflags, format);
	print_ipv6_addr(&cs, format);

	if (format & FMT_NOTABLE)
		fputs("  ", stdout);

	if (cs.fw6.ipv6.flags & IP6T_F_GOTO)
		printf("[goto] ");

	print_matches_and_target(&cs, format);

	if (!(format & FMT_NONEWLINE))
		fputc('\n', stdout);
}

static void save_ipv6_addr(char letter, const struct in6_addr *addr,
			   int invert)
{
	char addr_str[INET6_ADDRSTRLEN];

	if (!invert && IN6_IS_ADDR_UNSPECIFIED(addr))
		return;

	inet_ntop(AF_INET6, addr, addr_str, INET6_ADDRSTRLEN);
	printf("%s-%c %s ", invert ? "! " : "", letter, addr_str);
}

static void nft_ipv6_save_firewall(const void *data, unsigned int format)
{
	const struct iptables_command_state *cs = data;

	save_firewall_details(cs, cs->fw6.ipv6.invflags, cs->fw6.ipv6.proto,
			      cs->fw6.ipv6.iniface, cs->fw6.ipv6.iniface_mask,
			      cs->fw6.ipv6.outiface,
			      cs->fw6.ipv6.outiface_mask);

	save_ipv6_addr('s', &cs->fw6.ipv6.src,
		       cs->fw6.ipv6.invflags & IP6T_INV_SRCIP);
	save_ipv6_addr('d', &cs->fw6.ipv6.dst,
		       cs->fw6.ipv6.invflags & IP6T_INV_DSTIP);

	save_matches_and_target(cs->matches, cs->target,
				cs->jumpto, cs->fw6.ipv6.flags, &cs->fw6);

	if (cs->target == NULL && strlen(cs->jumpto) > 0) {
		printf("-%c %s", cs->fw6.ipv6.flags & IP6T_F_GOTO ? 'g' : 'j',
		       cs->jumpto);
	}
	printf("\n");
}

/* These are invalid numbers as upper layer protocol */
static int is_exthdr(uint16_t proto)
{
	return (proto == IPPROTO_ROUTING ||
		proto == IPPROTO_FRAGMENT ||
		proto == IPPROTO_AH ||
		proto == IPPROTO_DSTOPTS);
}

static void nft_ipv6_proto_parse(struct iptables_command_state *cs,
				 struct xtables_args *args)
{
	cs->fw6.ipv6.proto = args->proto;
	cs->fw6.ipv6.invflags = args->invflags;

	if (is_exthdr(cs->fw6.ipv6.proto)
	    && (cs->fw6.ipv6.invflags & XT_INV_PROTO) == 0)
		fprintf(stderr,
			"Warning: never matched protocol: %s. "
			"use extension match instead.\n",
			cs->protocol);
}

static void nft_ipv6_post_parse(int command, struct iptables_command_state *cs,
				struct xtables_args *args)
{
	if (args->proto != 0)
		args->flags |= IP6T_F_PROTO;

	cs->fw6.ipv6.flags = args->flags;
	/* We already set invflags in proto_parse, but we need to refresh it
	 * to include new parsed options.
	 */
	cs->fw6.ipv6.invflags = args->invflags;

	strncpy(cs->fw6.ipv6.iniface, args->iniface, IFNAMSIZ);
	memcpy(cs->fw6.ipv6.iniface_mask,
	       args->iniface_mask, IFNAMSIZ*sizeof(unsigned char));

	strncpy(cs->fw6.ipv6.outiface, args->outiface, IFNAMSIZ);
	memcpy(cs->fw6.ipv6.outiface_mask,
	       args->outiface_mask, IFNAMSIZ*sizeof(unsigned char));

	if (args->goto_set)
		cs->fw6.ipv6.flags |= IP6T_F_GOTO;

	cs->fw6.counters.pcnt = args->pcnt_cnt;
	cs->fw6.counters.bcnt = args->bcnt_cnt;

	if (command & (CMD_REPLACE | CMD_INSERT |
			CMD_DELETE | CMD_APPEND | CMD_CHECK)) {
		if (!(cs->options & OPT_DESTINATION))
			args->dhostnetworkmask = "::0/0";
		if (!(cs->options & OPT_SOURCE))
			args->shostnetworkmask = "::0/0";
	}

	if (args->shostnetworkmask)
		xtables_ip6parse_multiple(args->shostnetworkmask,
					  &args->s.addr.v6,
					  &args->s.mask.v6,
					  &args->s.naddrs);
	if (args->dhostnetworkmask)
		xtables_ip6parse_multiple(args->dhostnetworkmask,
					  &args->d.addr.v6,
					  &args->d.mask.v6,
					  &args->d.naddrs);

	if ((args->s.naddrs > 1 || args->d.naddrs > 1) &&
	    (cs->fw6.ipv6.invflags & (IP6T_INV_SRCIP | IP6T_INV_DSTIP)))
		xtables_error(PARAMETER_PROBLEM,
			      "! not allowed with multiple"
			      " source or destination IP addresses");
}

static void nft_ipv6_parse_target(struct xtables_target *t, void *data)
{
	struct iptables_command_state *cs = data;

	cs->target = t;
}

static bool nft_ipv6_rule_find(struct nft_family_ops *ops,
			       struct nftnl_rule *r, void *data)
{
	struct iptables_command_state *cs = data;

	return nft_ipv46_rule_find(ops, r, cs);
}

static void nft_ipv6_save_counters(const void *data)
{
	const struct iptables_command_state *cs = data;

	save_counters(cs->counters.pcnt, cs->counters.bcnt);
}

static void xlate_ipv6_addr(const char *selector, const struct in6_addr *addr,
			    const struct in6_addr *mask,
			    int invert, struct xt_xlate *xl)
{
	char addr_str[INET6_ADDRSTRLEN];

	if (!invert && IN6_IS_ADDR_UNSPECIFIED(addr))
		return;

	inet_ntop(AF_INET6, addr, addr_str, INET6_ADDRSTRLEN);
	xt_xlate_add(xl, "%s %s%s%s ", selector, invert ? "!= " : "", addr_str,
			xtables_ip6mask_to_numeric(mask));
}

static int nft_ipv6_xlate(const void *data, struct xt_xlate *xl)
{
	const struct iptables_command_state *cs = data;
	const char *comment;
	int ret;

	xlate_ifname(xl, "iifname", cs->fw6.ipv6.iniface,
		     cs->fw6.ipv6.invflags & IP6T_INV_VIA_IN);
	xlate_ifname(xl, "oifname", cs->fw6.ipv6.outiface,
		     cs->fw6.ipv6.invflags & IP6T_INV_VIA_OUT);

	if (cs->fw6.ipv6.proto != 0) {
		const struct protoent *pent =
			getprotobynumber(cs->fw6.ipv6.proto);
		char protonum[strlen("255") + 1];

		if (!xlate_find_match(cs, pent->p_name)) {
			snprintf(protonum, sizeof(protonum), "%u",
				 cs->fw6.ipv6.proto);
			protonum[sizeof(protonum) - 1] = '\0';
			xt_xlate_add(xl, "meta l4proto %s%s ",
				   cs->fw6.ipv6.invflags & IP6T_INV_PROTO ?
					"!= " : "",
				   pent ? pent->p_name : protonum);
		}
	}

	xlate_ipv6_addr("ip6 saddr", &cs->fw6.ipv6.src, &cs->fw6.ipv6.smsk,
			cs->fw6.ipv6.invflags & IP6T_INV_SRCIP, xl);
	xlate_ipv6_addr("ip6 daddr", &cs->fw6.ipv6.dst, &cs->fw6.ipv6.dmsk,
			cs->fw6.ipv6.invflags & IP6T_INV_DSTIP, xl);

	ret = xlate_matches(cs, xl);
	if (!ret)
		return ret;

	/* Always add counters per rule, as in iptables */
	xt_xlate_add(xl, "counter ");

	comment = xt_xlate_get_comment(xl);
	if (comment)
		xt_xlate_add(xl, "comment %s", comment);

	ret = xlate_action(cs, !!(cs->fw6.ipv6.flags & IP6T_F_GOTO), xl);

	return ret;
}

struct nft_family_ops nft_family_ops_ipv6 = {
	.add			= nft_ipv6_add,
	.is_same		= nft_ipv6_is_same,
	.parse_meta		= nft_ipv6_parse_meta,
	.parse_payload		= nft_ipv6_parse_payload,
	.parse_immediate	= nft_ipv6_parse_immediate,
	.print_header		= nft_ipv6_print_header,
	.print_firewall		= nft_ipv6_print_firewall,
	.save_firewall		= nft_ipv6_save_firewall,
	.save_counters		= nft_ipv6_save_counters,
	.proto_parse		= nft_ipv6_proto_parse,
	.post_parse		= nft_ipv6_post_parse,
	.parse_target		= nft_ipv6_parse_target,
	.rule_find		= nft_ipv6_rule_find,
	.xlate			= nft_ipv6_xlate,
};
