/*
 *  Copyright (c) 2010-2013, Rutgers, The State University of New Jersey
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the organization(s) stated above nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * MF_IPEncap.cc
 *
 *  Created on: May 10, 2013
 *      Author: Kiran Nagaraja
 * 
 * Adapted from Click IPEncap. Dst handling different that it made sense to
 * duplicate and repurpose the code.
 *
 */

#include <click/config.h>
#include <click/nameinfo.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>

#include "mfipencap.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_IPEncap::MF_IPEncap():logger(MF_Logger::init()) {
}

MF_IPEncap::~MF_IPEncap() {
}

int
MF_IPEncap::configure(Vector<String> &conf, ErrorHandler *errh) {
  click_ip iph;
  memset(&iph, 0, sizeof(click_ip));
  iph.ip_v = 4;
  iph.ip_hl = sizeof(click_ip) >> 2;
  iph.ip_ttl = 250;
  int proto, tos = -1, dscp = -1;
  bool ce = false, df = false;
  String ect_str, dst_str;

    if (Args(conf, this, errh)
	.read_mp("PROTO", NamedIntArg(NameInfo::T_IP_PROTO), proto)
	.read_mp("SRC", iph.ip_src)
	.read_mp("DST", AnyArg(), dst_str)
	.read("TOS", tos)
	.read("TTL", iph.ip_ttl)
	.read("DSCP", dscp)
	.read("ECT", KeywordArg(), ect_str)
	.read("CE", ce)
	.read("DF", df)
	.read("ARP_TABLE", ElementArg(), _ARPTable)
	.complete() < 0)
	return -1;

  if (proto < 0 || proto > 255)
      return errh->error("bad IP protocol");
  iph.ip_p = proto;

  bool use_nxthop_anno = dst_str == "NXTHOP_ANNO";
  if (use_nxthop_anno)
      iph.ip_dst.s_addr = 0;
  else if (!IPAddressArg().parse(dst_str, iph.ip_dst, this))
      return errh->error("DST argument should be IP address or 'NXTHOP_ANNO'");

  int ect = 0;
  if (ect_str) {
    bool x;
    if (BoolArg().parse(ect_str, x))
      ect = x;
    else if (ect_str == "2")
      ect = 2;
    else
      return errh->error("bad ECT value '%s'", ect_str.c_str());
  }

  if (tos >= 0 && dscp >= 0)
    return errh->error("cannot set both TOS and DSCP");
  else if (tos >= 0 && (ect || ce))
    return errh->error("cannot set both TOS and ECN bits");
  if (tos >= 256 || tos < -1)
    return errh->error("TOS too large; max 255");
  else if (dscp >= 63 || dscp < -1)
    return errh->error("DSCP too large; max 63");
  if (ect && ce)
    return errh->error("can set at most one ECN option");

  if (tos >= 0)
    iph.ip_tos = tos;
  else if (dscp >= 0)
    iph.ip_tos = (dscp << 2);
  if (ect)
    iph.ip_tos |= (ect == 1 ? IP_ECN_ECT1 : IP_ECN_ECT2);
  if (ce)
    iph.ip_tos |= IP_ECN_CE;
  if (df)
    iph.ip_off |= htons(IP_DF);
  _iph = iph;

  // set the checksum field so we can calculate the checksum incrementally
#if HAVE_FAST_CHECKSUM
  _iph.ip_sum = ip_fast_csum((unsigned char *) &_iph, sizeof(click_ip) >> 2);
#else
  _iph.ip_sum = click_in_cksum((unsigned char *) &_iph, sizeof(click_ip));
#endif

  // store information about use_dst_anno in the otherwise useless
  // _iph.ip_len field
  _iph.ip_len = (use_nxthop_anno ? 1 : 0);

  return 0;
}

int
MF_IPEncap::initialize(ErrorHandler *) {
  _id = 0;
  return 0;
}

inline void
MF_IPEncap::update_cksum(click_ip *ip, int off) const {
#if HAVE_INDIFFERENT_ALIGNMENT
    click_update_in_cksum(&ip->ip_sum, 0, ((uint16_t *) ip)[off/2]);
#else
    const uint8_t *u = (const uint8_t *) ip;
# if CLICK_BYTE_ORDER == CLICK_BIG_ENDIAN
    click_update_in_cksum(&ip->ip_sum, 0, u[off]*256 + u[off+1]);
# else
    click_update_in_cksum(&ip->ip_sum, 0, u[off] + u[off+1]*256);
# endif
#endif
}

Packet *
MF_IPEncap::simple_action(Packet *p_in) {
  WritablePacket *p = p_in->push(sizeof(click_ip));
  if (!p) return 0;

  click_ip *ip = reinterpret_cast<click_ip *>(p->data());
  memcpy(ip, &_iph, sizeof(click_ip));
  if (ip->ip_len) {		// use nxthop_anno
      uint32_t next_hop_GUID = p->anno_u32(NXTHOP_ANNO_OFFSET);
	if (!next_hop_GUID) {
                //log set to debug since link state packets commonly use this
		logger.log(MF_DEBUG, 
		"ipencap: Nexthop GUID anno not set or broadcast. "
                "Dst IP set to broadcast!");
                ip->ip_dst = IPAddress::make_broadcast().in_addr();
	} else {
		IPAddress dst = ((MF_ARPTable*)_ARPTable)->getIP(next_hop_GUID);
		if (dst.addr() != 0) {
			ip->ip_dst = dst.in_addr();
		} else {
			logger.log(MF_ERROR, 
			"ipencap: No IP found for GUID %u. Dropping!",
			next_hop_GUID);
                        p->kill();
                        return 0; 
		}
	}
	update_cksum(ip, 16);
	update_cksum(ip, 18);
  } else
      p->set_dst_ip_anno(IPAddress(ip->ip_dst));
  ip->ip_len = htons(p->length());
  ip->ip_id = htons(_id.fetch_and_add(1));
  update_cksum(ip, 2);
  update_cksum(ip, 4);

  p->set_ip_header(ip, sizeof(click_ip));

  return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_IPEncap)
ELEMENT_MT_SAFE(MF_IPEncap)
