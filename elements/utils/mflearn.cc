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
 * MF_Learn.cc
 *
 *  Created on: May 10, 2013
 *      Author: Kiran Nagaraja
 * 
 * Learns details about the packet source as identified by the GUID
 * specified in the source field of the packet.
 * Mappings of GUID to router ingress port, src MAC and IP addresses are 
 * are posted to the ARP table for automated egress packet switching.
 */

#include <click/config.h>
#include <click/nameinfo.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>

#include "mflearn.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS
MF_Learn::MF_Learn():logger(MF_Logger::init()) {
}

MF_Learn::~MF_Learn() {

}

int
MF_Learn::configure(Vector<String> &conf, ErrorHandler *errh) {

    if (Args(conf, this, errh)
	.read_mp("IN_PORT", in_port)
	.read("ARP_TABLE", ElementArg(), arp_tbl_p)
	.complete() < 0)
	return -1;
  return 0;
}

int
MF_Learn::initialize(ErrorHandler *) {

    return 0;
}

Packet *
MF_Learn::simple_action(Packet *p) {

    if(!p) return 0;

    //Assumes these packts are either MF-over-eth, or MF-over-IP-over-eth

    const click_ether *eh = (const click_ether *) p->data();
    EtherAddress src_mac = EtherAddress(eh->ether_shost);
    rcvd_pkt * mf_hdr = (rcvd_pkt *) (p->data() + 14); 
    const click_ip *iph;
    IPAddress src_ip;	

    //if ether type is IPv4 with IP proto being MF(==5), then
    //this is a MF-in-IPv4 packet. 

    bool has_ip = false;
    if (ntohs(eh->ether_type) == 0x0800) {
        has_ip = true;
        iph = (const click_ip *) (p->data() + 14);	
        src_ip = IPAddress(iph->ip_src);

        mf_hdr = (rcvd_pkt *) (p->data() + 34);
    }

    uint32_t type = ntohl(mf_hdr->type);

    /**
     * Only control pkts have src guid in header.
     * Most data pkts may have only hop/link header, 
     * with only the first pkt in a chunk transfer
     * having the entire L3 header with src/dst GUIDs
     * 
     * We will use the ack pkt to a link probe message as
     * the basic event to learn details about pkt src. 
     */
    if (type == LP_ACK_PKT) {

        uint32_t src_guid = ntohl(mf_hdr->source);

        //save src GUID to MAC, IP, and ingress port mappings
        ((MF_ARPTable*)arp_tbl_p)->setMAC(src_guid, src_mac);
        if (has_ip) {
            ((MF_ARPTable*)arp_tbl_p)->setIP(src_guid, src_ip);
        }
        ((MF_ARPTable*)arp_tbl_p)->setPort(src_guid, in_port);
        logger.log(MF_DEBUG, 
            "learn: GUID %u --> mac:ip %s:%s ingress port: %u", 
            src_guid, 
            src_mac.unparse().c_str(), 
            (has_ip)?src_ip.unparse().c_str():"-", in_port);
    }

    //set pkt annotations using previously learnt info
    /* ingress port */
    p->set_anno_u8(IN_PORT_ANNO_OFFSET, in_port);

    /* src GUID - reverse lookup on src MAC */
    uint32_t src_id = ((MF_ARPTable*)arp_tbl_p)->getGUID(src_mac);
    /* if no association found then returned guid is 0 - invalid */
    p->set_anno_u32(SRC_GUID_ANNO_OFFSET, src_id);

    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_Learn)
ELEMENT_MT_SAFE(MF_Learn)
