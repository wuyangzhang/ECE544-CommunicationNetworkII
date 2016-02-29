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
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include "mfetherdrop.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_EtherDrop::MF_EtherDrop():logger(MF_Logger::init()) {
}

MF_EtherDrop::~MF_EtherDrop() {
}

int MF_EtherDrop::configure(Vector<String> &conf, ErrorHandler *errh) {
	memset(_ethh2.ether_shost, 0, 6);
	if(cp_va_kparse(conf, this, errh,
		"ARP_TABLE", cpkP+cpkM, cpElement, &_ARPTable,
		/* ether_shost is of uint8_t host[6] */	
		"SRC", cpkP+cpkM, cpEthernetAddress, &_ethh.ether_shost, 
		"SRC2", cpkP, cpEthernetAddress, &_ethh2.ether_shost, 
                    cpEnd) < 0)
		return -1;
	return 0;
}

void MF_EtherDrop::push(int port, Packet *p) {
	
	assert(port==0);
	int num_dev = 1;
	uint8_t zeros[6];
	memset(zeros, 0, 6);
	if(memcmp(_ethh2.ether_shost, zeros, 6))  {/* not equal */
		num_dev++;
	}
	//logger.log(MF_DEBUG, "eth_drop: num_dev: %d", num_dev);

	// Get the incoming packet's src mac address
	const click_ether *ethh = (const click_ether*)p->mac_header();

	EtherAddress my_ether(_ethh.ether_shost);	

	EtherAddress src_ether(ethh->ether_shost);
	EtherAddress dst_ether(ethh->ether_dhost);

	/*
	logger.log(MF_DEBUG, "eth_drop: pkt smac %s, dmac %s", 
		src_ether.unparse().c_str(), 
		dst_ether.unparse().c_str());
	*/

	if(num_dev==1) {
		/*
		logger.log(MF_DEBUG, "eth_drop: ethh1 mac %s", 
				my_ether.unparse().c_str());
		*/
		if(_ARPTable->getGUID(src_ether)==0)
			p->kill();
		else if(my_ether!=dst_ether)
			p->kill();
		else  
			output(0).push(p);
	}
	else {
		EtherAddress my_ether2(_ethh2.ether_shost);	
		/*
		logger.log(MF_DEBUG, "eth_drop: ethh2 mac %s", 
				my_ether2.unparse().c_str());
		*/

                if(_ARPTable->getGUID(src_ether)==0)
                        p->kill();
                else if(my_ether!=dst_ether && my_ether2!=dst_ether)
                        p->kill();
                else
                        output(0).push(p);
	}
}


CLICK_DECLS
EXPORT_ELEMENT(MF_EtherDrop)
ELEMENT_REQUIRES(userlevel)
