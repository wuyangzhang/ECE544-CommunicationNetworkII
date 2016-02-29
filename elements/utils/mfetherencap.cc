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
 * MF_EtherEncap.cc
 *
 *  Created on: Jul 16, 2011
 *      Author: sk
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include "mfetherencap.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_EtherEncap::MF_EtherEncap():logger(MF_Logger::init()) {
}

MF_EtherEncap::~MF_EtherEncap() {
}

int MF_EtherEncap::configure(Vector<String> &conf, ErrorHandler *errh) {
    unsigned etht;
    if(cp_va_kparse(conf, this, errh,
                "ETHERTYPE", cpkP+cpkM, cpUnsigned, &etht,
                "SRC", cpkP+cpkM, cpEthernetAddress, &_ethh.ether_shost,
                "ARP_TABLE", cpkP+cpkM, cpElement, &_ARPTable,
                cpEnd) < 0)
        return -1;
    _ethh.ether_type = htons(etht);
    return 0;
}

void MF_EtherEncap::push(int port, Packet *p) {
    if(port != 0){
        logger.log(MF_FATAL, 
            "eth_encap: Packet received on unsupported port");
        exit(-1);
    }
    /* Check if it's a broadcast message */
    uint8_t out_port = p->anno_u8(OUT_PORT_ANNO_OFFSET);

    EtherAddress ether_addr;

    if(out_port == 255) {
        ether_addr = EtherAddress::make_broadcast();	
    }
    else {
        uint32_t next_hop_GUID = p->anno_u32(NXTHOP_ANNO_OFFSET);
        ether_addr = _ARPTable->getMAC(next_hop_GUID);
    }

    memcpy(_ethh.ether_dhost, ether_addr.data(), 6);
    if (WritablePacket *q = p->push_mac_header(14)) {
        memcpy(q->data(), &_ethh, 14);
        output(0).push(q);
    }
}

CLICK_DECLS
EXPORT_ELEMENT(MF_EtherEncap)
ELEMENT_REQUIRES(userlevel)
