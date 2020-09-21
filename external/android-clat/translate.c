/*
 * Copyright 2011 Daniel Drown
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * translate.c - CLAT functions / partial implementation of rfc6145
 */
#include <string.h>

#include "icmp.h"
#include "translate.h"
#include "checksum.h"
#include "clatd.h"
#include "config.h"
#include "logging.h"
#include "debug.h"
#include "tun.h"

/* function: packet_checksum
 * calculates the checksum over all the packet components starting from pos
 * checksum - checksum of packet components before pos
 * packet   - packet to calculate the checksum of
 * pos      - position to start counting from
 * returns  - the completed 16-bit checksum, ready to write into a checksum header field
 */
uint16_t packet_checksum(uint32_t checksum, clat_packet packet, clat_packet_index pos) {
  int i;
  for (i = pos; i < CLAT_POS_MAX; i++) {
    if (packet[i].iov_len > 0) {
      checksum = ip_checksum_add(checksum, packet[i].iov_base, packet[i].iov_len);
    }
  }
  return ip_checksum_finish(checksum);
}

/* function: packet_length
 * returns the total length of all the packet components after pos
 * packet - packet to calculate the length of
 * pos    - position to start counting after
 * returns: the total length of the packet components after pos
 */
uint16_t packet_length(clat_packet packet, clat_packet_index pos) {
  size_t len = 0;
  int i;
  for (i = pos + 1; i < CLAT_POS_MAX; i++) {
    len += packet[i].iov_len;
  }
  return len;
}

/* function: is_in_plat_subnet
 * returns true iff the given IPv6 address is in the plat subnet.
 * addr - IPv6 address
 */
int is_in_plat_subnet(const struct in6_addr *addr6) {
  // Assumes a /96 plat subnet.
  return (addr6 != NULL) && (memcmp(addr6, &Global_Clatd_Config.plat_subnet, 12) == 0);
}

/* function: ipv6_addr_to_ipv4_addr
 * return the corresponding ipv4 address for the given ipv6 address
 * addr6 - ipv6 address
 * returns: the IPv4 address
 */
uint32_t ipv6_addr_to_ipv4_addr(const struct in6_addr *addr6) {
  if (is_in_plat_subnet(addr6)) {
    // Assumes a /96 plat subnet.
    return addr6->s6_addr32[3];
  } else if (IN6_ARE_ADDR_EQUAL(addr6, &Global_Clatd_Config.ipv6_local_subnet)) {
    // Special-case our own address.
    return Global_Clatd_Config.ipv4_local_subnet.s_addr;
  } else {
    // Third party packet. Let the caller deal with it.
    return INADDR_NONE;
  }
}

/* function: ipv4_addr_to_ipv6_addr
 * return the corresponding ipv6 address for the given ipv4 address
 * addr4 - ipv4 address
 */
struct in6_addr ipv4_addr_to_ipv6_addr(uint32_t addr4) {
  struct in6_addr addr6;
  // Both addresses are in network byte order (addr4 comes from a network packet, and the config
  // file entry is read using inet_ntop).
  if (addr4 == Global_Clatd_Config.ipv4_local_subnet.s_addr) {
    return Global_Clatd_Config.ipv6_local_subnet;
  } else {
    // Assumes a /96 plat subnet.
    addr6 = Global_Clatd_Config.plat_subnet;
    addr6.s6_addr32[3] = addr4;
    return addr6;
  }
}

/* function: fill_tun_header
 * fill in the header for the tun fd
 * tun_header - tunnel header, already allocated
 * proto      - ethernet protocol id: ETH_P_IP(ipv4) or ETH_P_IPV6(ipv6)
 */
void fill_tun_header(struct tun_pi *tun_header, uint16_t proto) {
  tun_header->flags = 0;
  tun_header->proto = htons(proto);
}

/* function: fill_ip_header
 * generate an ipv4 header from an ipv6 header
 * ip_targ     - (ipv4) target packet header, source: original ipv4 addr, dest: local subnet addr
 * payload_len - length of other data inside packet
 * protocol    - protocol number (tcp, udp, etc)
 * old_header  - (ipv6) source packet header, source: nat64 prefix, dest: local subnet prefix
 */
void fill_ip_header(struct iphdr *ip, uint16_t payload_len, uint8_t protocol,
                    const struct ip6_hdr *old_header) {
  int ttl_guess;
  memset(ip, 0, sizeof(struct iphdr));

  ip->ihl = 5;
  ip->version = 4;
  ip->tos = 0;
  ip->tot_len = htons(sizeof(struct iphdr) + payload_len);
  ip->id = 0;
  ip->frag_off = htons(IP_DF);
  ip->ttl = old_header->ip6_hlim;
  ip->protocol = protocol;
  ip->check = 0;

  ip->saddr = ipv6_addr_to_ipv4_addr(&old_header->ip6_src);
  ip->daddr = ipv6_addr_to_ipv4_addr(&old_header->ip6_dst);

  // Third-party ICMPv6 message. This may have been originated by an native IPv6 address.
  // In that case, the source IPv6 address can't be translated and we need to make up an IPv4
  // source address. For now, use 255.0.0.<ttl>, which at least looks useful in traceroute.
  if ((uint32_t) ip->saddr == INADDR_NONE) {
    ttl_guess = icmp_guess_ttl(old_header->ip6_hlim);
    ip->saddr = htonl((0xff << 24) + ttl_guess);
  }
}

/* function: fill_ip6_header
 * generate an ipv6 header from an ipv4 header
 * ip6         - (ipv6) target packet header, source: local subnet prefix, dest: nat64 prefix
 * payload_len - length of other data inside packet
 * protocol    - protocol number (tcp, udp, etc)
 * old_header  - (ipv4) source packet header, source: local subnet addr, dest: internet's ipv4 addr
 */
void fill_ip6_header(struct ip6_hdr *ip6, uint16_t payload_len, uint8_t protocol,
                     const struct iphdr *old_header) {
  memset(ip6, 0, sizeof(struct ip6_hdr));

  ip6->ip6_vfc = 6 << 4;
  ip6->ip6_plen = htons(payload_len);
  ip6->ip6_nxt = protocol;
  ip6->ip6_hlim = old_header->ttl;

  ip6->ip6_src = ipv4_addr_to_ipv6_addr(old_header->saddr);
  ip6->ip6_dst = ipv4_addr_to_ipv6_addr(old_header->daddr);
}

/* function: maybe_fill_frag_header
 * fills a fragmentation header
 * generate an ipv6 fragment header from an ipv4 header
 * frag_hdr    - target (ipv6) fragmentation header
 * ip6_targ    - target (ipv6) header
 * old_header  - (ipv4) source packet header
 * returns: the length of the fragmentation header if present, or zero if not present
 */
size_t maybe_fill_frag_header(struct ip6_frag *frag_hdr, struct ip6_hdr *ip6_targ,
                              const struct iphdr *old_header) {
  uint16_t frag_flags = ntohs(old_header->frag_off);
  uint16_t frag_off = frag_flags & IP_OFFMASK;
  if (frag_off == 0 && (frag_flags & IP_MF) == 0) {
    // Not a fragment.
    return 0;
  }

  frag_hdr->ip6f_nxt = ip6_targ->ip6_nxt;
  frag_hdr->ip6f_reserved = 0;
  // In IPv4, the offset is the bottom 13 bits; in IPv6 it's the top 13 bits.
  frag_hdr->ip6f_offlg = htons(frag_off << 3);
  if (frag_flags & IP_MF) {
    frag_hdr->ip6f_offlg |= IP6F_MORE_FRAG;
  }
  frag_hdr->ip6f_ident = htonl(ntohs(old_header->id));
  ip6_targ->ip6_nxt = IPPROTO_FRAGMENT;

  return sizeof(*frag_hdr);
}

/* function: parse_frag_header
 * return the length of the fragmentation header if present, or zero if not present
 * generate an ipv6 fragment header from an ipv4 header
 * frag_hdr    - (ipv6) fragmentation header
 * ip_targ     - target (ipv4) header
 * returns: the next header value
 */
uint8_t parse_frag_header(const struct ip6_frag *frag_hdr, struct iphdr *ip_targ) {
  uint16_t frag_off = (ntohs(frag_hdr->ip6f_offlg & IP6F_OFF_MASK) >> 3);
  if (frag_hdr->ip6f_offlg & IP6F_MORE_FRAG) {
    frag_off |= IP_MF;
  }
  ip_targ->frag_off = htons(frag_off);
  ip_targ->id = htons(ntohl(frag_hdr->ip6f_ident) & 0xffff);
  ip_targ->protocol = frag_hdr->ip6f_nxt;
  return frag_hdr->ip6f_nxt;
}

/* function: icmp_to_icmp6
 * translate ipv4 icmp to ipv6 icmp
 * out          - output packet
 * icmp         - source packet icmp header
 * checksum     - pseudo-header checksum
 * payload      - icmp payload
 * payload_size - size of payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int icmp_to_icmp6(clat_packet out, clat_packet_index pos, const struct icmphdr *icmp,
                  uint32_t checksum, const uint8_t *payload, size_t payload_size) {
  struct icmp6_hdr *icmp6_targ = out[pos].iov_base;
  uint8_t icmp6_type;
  int clat_packet_len;

  memset(icmp6_targ, 0, sizeof(struct icmp6_hdr));

  icmp6_type = icmp_to_icmp6_type(icmp->type, icmp->code);
  icmp6_targ->icmp6_type = icmp6_type;
  icmp6_targ->icmp6_code = icmp_to_icmp6_code(icmp->type, icmp->code);

  out[pos].iov_len = sizeof(struct icmp6_hdr);

  if (pos == CLAT_POS_TRANSPORTHDR &&
      is_icmp_error(icmp->type) &&
      icmp6_type != ICMP6_PARAM_PROB) {
    // An ICMP error we understand, one level deep.
    // Translate the nested packet (the one that caused the error).
    clat_packet_len = ipv4_packet(out, pos + 1, payload, payload_size);

    // The pseudo-header checksum was calculated on the transport length of the original IPv4
    // packet that we were asked to translate. This transport length is 20 bytes smaller than it
    // needs to be, because the ICMP error contains an IPv4 header, which we will be translating to
    // an IPv6 header, which is 20 bytes longer. Fix it up here.
    // We only need to do this for ICMP->ICMPv6, not ICMPv6->ICMP, because ICMP does not use the
    // pseudo-header when calculating its checksum (as the IPv4 header has its own checksum).
    checksum = checksum + htons(20);
  } else if (icmp6_type == ICMP6_ECHO_REQUEST || icmp6_type == ICMP6_ECHO_REPLY) {
    // Ping packet.
    icmp6_targ->icmp6_id = icmp->un.echo.id;
    icmp6_targ->icmp6_seq = icmp->un.echo.sequence;
    out[CLAT_POS_PAYLOAD].iov_base = (uint8_t *) payload;
    out[CLAT_POS_PAYLOAD].iov_len = payload_size;
    clat_packet_len = CLAT_POS_PAYLOAD + 1;
  } else {
    // Unknown type/code. The type/code conversion functions have already logged an error.
    return 0;
  }

  icmp6_targ->icmp6_cksum = 0;  // Checksum field must be 0 when calculating checksum.
  icmp6_targ->icmp6_cksum = packet_checksum(checksum, out, pos);

  return clat_packet_len;
}

/* function: icmp6_to_icmp
 * translate ipv6 icmp to ipv4 icmp
 * out          - output packet
 * icmp6        - source packet icmp6 header
 * payload      - icmp6 payload
 * payload_size - size of payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int icmp6_to_icmp(clat_packet out, clat_packet_index pos, const struct icmp6_hdr *icmp6,
                  const uint8_t *payload, size_t payload_size) {
  struct icmphdr *icmp_targ = out[pos].iov_base;
  uint8_t icmp_type;
  int clat_packet_len;

  memset(icmp_targ, 0, sizeof(struct icmphdr));

  icmp_type = icmp6_to_icmp_type(icmp6->icmp6_type, icmp6->icmp6_code);
  icmp_targ->type = icmp_type;
  icmp_targ->code = icmp6_to_icmp_code(icmp6->icmp6_type, icmp6->icmp6_code);

  out[pos].iov_len = sizeof(struct icmphdr);

  if (pos == CLAT_POS_TRANSPORTHDR &&
      is_icmp6_error(icmp6->icmp6_type) &&
      icmp_type != ICMP_PARAMETERPROB) {
    // An ICMPv6 error we understand, one level deep.
    // Translate the nested packet (the one that caused the error).
    clat_packet_len = ipv6_packet(out, pos + 1, payload, payload_size);
  } else if (icmp_type == ICMP_ECHO || icmp_type == ICMP_ECHOREPLY) {
    // Ping packet.
    icmp_targ->un.echo.id = icmp6->icmp6_id;
    icmp_targ->un.echo.sequence = icmp6->icmp6_seq;
    out[CLAT_POS_PAYLOAD].iov_base = (uint8_t *) payload;
    out[CLAT_POS_PAYLOAD].iov_len = payload_size;
    clat_packet_len = CLAT_POS_PAYLOAD + 1;
  } else {
      // Unknown type/code. The type/code conversion functions have already logged an error.
    return 0;
  }

  icmp_targ->checksum = 0;  // Checksum field must be 0 when calculating checksum.
  icmp_targ->checksum = packet_checksum(0, out, pos);

  return clat_packet_len;
}

/* function: generic_packet
 * takes a generic IP packet and sets it up for translation
 * out      - output packet
 * pos      - position in the output packet of the transport header
 * payload  - pointer to IP payload
 * len      - size of ip payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int generic_packet(clat_packet out, clat_packet_index pos, const uint8_t *payload, size_t len) {
  out[pos].iov_len = 0;
  out[CLAT_POS_PAYLOAD].iov_base = (uint8_t *) payload;
  out[CLAT_POS_PAYLOAD].iov_len = len;

  return CLAT_POS_PAYLOAD + 1;
}

/* function: udp_packet
 * takes a udp packet and sets it up for translation
 * out      - output packet
 * udp      - pointer to udp header in packet
 * old_sum  - pseudo-header checksum of old header
 * new_sum  - pseudo-header checksum of new header
 * len      - size of ip payload
 */
int udp_packet(clat_packet out, clat_packet_index pos, const struct udphdr *udp,
               uint32_t old_sum, uint32_t new_sum, size_t len) {
  const uint8_t *payload;
  size_t payload_size;

  if(len < sizeof(struct udphdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR,"udp_packet/(too small)");
    return 0;
  }

  payload = (const uint8_t *) (udp + 1);
  payload_size = len - sizeof(struct udphdr);

  return udp_translate(out, pos, udp, old_sum, new_sum, payload, payload_size);
}

/* function: tcp_packet
 * takes a tcp packet and sets it up for translation
 * out      - output packet
 * tcp      - pointer to tcp header in packet
 * checksum - pseudo-header checksum
 * len      - size of ip payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int tcp_packet(clat_packet out, clat_packet_index pos, const struct tcphdr *tcp,
               uint32_t old_sum, uint32_t new_sum, size_t len) {
  const uint8_t *payload;
  size_t payload_size, header_size;

  if(len < sizeof(struct tcphdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR,"tcp_packet/(too small)");
    return 0;
  }

  if(tcp->doff < 5) {
    logmsg_dbg(ANDROID_LOG_ERROR,"tcp_packet/tcp header length set to less than 5: %x", tcp->doff);
    return 0;
  }

  if((size_t) tcp->doff*4 > len) {
    logmsg_dbg(ANDROID_LOG_ERROR,"tcp_packet/tcp header length set too large: %x", tcp->doff);
    return 0;
  }

  header_size = tcp->doff * 4;
  payload = ((const uint8_t *) tcp) + header_size;
  payload_size = len - header_size;

  return tcp_translate(out, pos, tcp, header_size, old_sum, new_sum, payload, payload_size);
}

/* function: udp_translate
 * common between ipv4/ipv6 - setup checksum and send udp packet
 * out          - output packet
 * udp          - udp header
 * old_sum      - pseudo-header checksum of old header
 * new_sum      - pseudo-header checksum of new header
 * payload      - tcp payload
 * payload_size - size of payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int udp_translate(clat_packet out, clat_packet_index pos, const struct udphdr *udp,
                  uint32_t old_sum, uint32_t new_sum, const uint8_t *payload, size_t payload_size) {
  struct udphdr *udp_targ = out[pos].iov_base;

  memcpy(udp_targ, udp, sizeof(struct udphdr));

  out[pos].iov_len = sizeof(struct udphdr);
  out[CLAT_POS_PAYLOAD].iov_base = (uint8_t *) payload;
  out[CLAT_POS_PAYLOAD].iov_len = payload_size;

  if (udp_targ->check) {
    udp_targ->check = ip_checksum_adjust(udp->check, old_sum, new_sum);
  } else {
    // Zero checksums are special. RFC 768 says, "An all zero transmitted checksum value means that
    // the transmitter generated no checksum (for debugging or for higher level protocols that
    // don't care)." However, in IPv6 zero UDP checksums were only permitted by RFC 6935 (2013). So
    // for safety we recompute it.
    udp_targ->check = 0;  // Checksum field must be 0 when calculating checksum.
    udp_targ->check = packet_checksum(new_sum, out, pos);
  }

  // RFC 768: "If the computed checksum is zero, it is transmitted as all ones (the equivalent
  // in one's complement arithmetic)."
  if (!udp_targ->check) {
    udp_targ->check = 0xffff;
  }

  return CLAT_POS_PAYLOAD + 1;
}

/* function: tcp_translate
 * common between ipv4/ipv6 - setup checksum and send tcp packet
 * out          - output packet
 * tcp          - tcp header
 * header_size  - size of tcp header including options
 * checksum     - partial checksum covering ipv4/ipv6 header
 * payload      - tcp payload
 * payload_size - size of payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int tcp_translate(clat_packet out, clat_packet_index pos, const struct tcphdr *tcp,
                  size_t header_size, uint32_t old_sum, uint32_t new_sum,
                  const uint8_t *payload, size_t payload_size) {
  struct tcphdr *tcp_targ = out[pos].iov_base;
  out[pos].iov_len = header_size;

  if (header_size > MAX_TCP_HDR) {
    // A TCP header cannot be more than MAX_TCP_HDR bytes long because it's a 4-bit field that
    // counts in 4-byte words. So this can never happen unless there is a bug in the caller.
    logmsg(ANDROID_LOG_ERROR, "tcp_translate: header too long %d > %d, truncating",
           header_size, MAX_TCP_HDR);
    header_size = MAX_TCP_HDR;
  }

  memcpy(tcp_targ, tcp, header_size);

  out[CLAT_POS_PAYLOAD].iov_base = (uint8_t *) payload;
  out[CLAT_POS_PAYLOAD].iov_len = payload_size;

  tcp_targ->check = ip_checksum_adjust(tcp->check, old_sum, new_sum);

  return CLAT_POS_PAYLOAD + 1;
}

// Weak symbol so we can override it in the unit test.
void send_rawv6(int fd, clat_packet out, int iov_len) __attribute__((weak));

void send_rawv6(int fd, clat_packet out, int iov_len) {
  // A send on a raw socket requires a destination address to be specified even if the socket's
  // protocol is IPPROTO_RAW. This is the address that will be used in routing lookups; the
  // destination address in the packet header only affects what appears on the wire, not where the
  // packet is sent to.
  static struct sockaddr_in6 sin6 = { AF_INET6, 0, 0, { { { 0, 0, 0, 0 } } }, 0 };
  static struct msghdr msg = {
    .msg_name = &sin6,
    .msg_namelen = sizeof(sin6),
  };

  msg.msg_iov = out,
  msg.msg_iovlen = iov_len,
  sin6.sin6_addr = ((struct ip6_hdr *) out[CLAT_POS_IPHDR].iov_base)->ip6_dst;
  sendmsg(fd, &msg, 0);
}

/* function: translate_packet
 * takes a packet, translates it, and writes it to fd
 * fd         - fd to write translated packet to
 * to_ipv6    - true if translating to ipv6, false if translating to ipv4
 * packet     - packet
 * packetsize - size of packet
 */
void translate_packet(int fd, int to_ipv6, const uint8_t *packet, size_t packetsize) {
  int iov_len = 0;

  // Allocate buffers for all packet headers.
  struct tun_pi tun_targ;
  char iphdr[sizeof(struct ip6_hdr)];
  char fraghdr[sizeof(struct ip6_frag)];
  char transporthdr[MAX_TCP_HDR];
  char icmp_iphdr[sizeof(struct ip6_hdr)];
  char icmp_fraghdr[sizeof(struct ip6_frag)];
  char icmp_transporthdr[MAX_TCP_HDR];

  // iovec of the packets we'll send. This gets passed down to the translation functions.
  clat_packet out = {
    { &tun_targ, 0 },                 // Tunnel header.
    { iphdr, 0 },                     // IP header.
    { fraghdr, 0 },                   // Fragment header.
    { transporthdr, 0 },              // Transport layer header.
    { icmp_iphdr, 0 },                // ICMP error inner IP header.
    { icmp_fraghdr, 0 },              // ICMP error fragmentation header.
    { icmp_transporthdr, 0 },         // ICMP error transport layer header.
    { NULL, 0 },                      // Payload. No buffer, it's a pointer to the original payload.
  };

  if (to_ipv6) {
    iov_len = ipv4_packet(out, CLAT_POS_IPHDR, packet, packetsize);
    if (iov_len > 0) {
      send_rawv6(fd, out, iov_len);
    }
  } else {
    iov_len = ipv6_packet(out, CLAT_POS_IPHDR, packet, packetsize);
    if (iov_len > 0) {
      fill_tun_header(&tun_targ, ETH_P_IP);
      out[CLAT_POS_TUNHDR].iov_len = sizeof(tun_targ);
      send_tun(fd, out, iov_len);
    }
  }
}
