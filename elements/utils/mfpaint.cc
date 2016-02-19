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
 * MF_Paint.cc
 *
 *  Created on: March 9th, 2012
 *      Author: sk
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/straccum.hh>
#include <click/args.hh>
#include <click/handlercall.hh>

#include "mfpaint.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_Paint::MF_Paint():logger(MF_Logger::init()) {
}

MF_Paint::~MF_Paint() {
}

int MF_Paint::configure(Vector<String> &conf, ErrorHandler *errh) {

    if(cp_va_kparse(conf, this, errh,
                "ARP_TABLE", cpkP+cpkM, cpElement, &_ARPTable,
                cpEnd) < 0)
        return -1;
    return 0;
}

void MF_Paint::push(int port, Packet *p) {

    if(port != 0){
        logger.log(MF_FATAL, 
            "paint: Packet received on unsupported port");
        exit(-1);
    }

    uint32_t next_hop_guid = p->anno_u32(NXTHOP_ANNO_OFFSET);

    uint8_t rtr_port = _ARPTable->getPort(next_hop_guid);
    p->set_anno_u8(OUT_PORT_ANNO_OFFSET, rtr_port);

    //update port/nexthop nbr traffic stats
    if(traffic_map_t::Pair *kvp = trf_map.find_pair(next_hop_guid)) {
        kvp->value.port_id = rtr_port;
        kvp->value.out_pkts++;
        kvp->value.out_bytes += p->length();
    } else {
        nbr_stats_t ns;
        ns.nbr_guid = next_hop_guid;
        ns.port_id = rtr_port;
        ns.out_pkts = 1;
        ns.out_bytes = p->length();
        trf_map.insert(next_hop_guid, ns);
    }

    output(0).push(p);
}

String MF_Paint::read_handler(Element *e, void *thunk) {

	MF_Paint *pnt= (MF_Paint *)e;
	StringAccum buf;

	switch((intptr_t)thunk) {
	    case READ_TRAFFIC_TABLE:
		for(traffic_map_t::const_iterator 
                    iter = pnt->trf_map.begin();
                    iter != pnt->trf_map.end(); ++iter) {

		    nbr_stats_t ns = iter.value();
                    buf << "[" << ns.nbr_guid << ",";
                    buf << ns.port_id << ",";
                    buf << ns.out_pkts << ",";
                    buf << ns.out_bytes << "],";
		}
	    default:
		buf << "[]";	
	}

	return buf.take_string();
}

void MF_Paint::add_handlers() {

    add_read_handler("port_traffic_table", read_handler, 
        (void *)READ_TRAFFIC_TABLE);
}

CLICK_DECLS
EXPORT_ELEMENT(MF_Paint)
ELEMENT_REQUIRES(userlevel)
