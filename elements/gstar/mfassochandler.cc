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
 * MF_AssocHandler.cc
 *
 *  Created on: Oct 19, 2011
 *      Author: sk
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include "mfassochandler.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_AssocHandler::MF_AssocHandler():logger(MF_Logger::init()) {
}

MF_AssocHandler::~MF_AssocHandler() {
}

int MF_AssocHandler::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
        "ASSOC_TABLE", cpkP+cpkM, cpElement, &_assocTable,
        cpEnd) < 0) {
    return -1;
  }
  return 0;
}
void MF_AssocHandler::push(int port, Packet *p) {
  if (port == 0) {
    handleAssocPkt(p); 
  } else {
    logger.log(MF_FATAL,
      "host_assoc: Packet received on unsupported port"); 
  }
}

void MF_AssocHandler::handleAssocPkt(Packet *p) {
  client_assoc_pkt *assoc_p = (client_assoc_pkt *)(p->data());
  uint32_t host_GUID = ntohl(assoc_p->host_GUID); 
  uint32_t assoc_GUID = ntohl(assoc_p->client_GUID); 
  logger.log(MF_DEBUG, 
    "host_assoc: Recvd assoc. message host guid: %u c-guid: %u", 
    host_GUID, assoc_GUID); 
  _assocTable->insert(host_GUID, assoc_GUID);
  //_assocTable->print();  
  WritablePacket *q = Packet::make(0, 0, sizeof(gnrs_req), 0);
  if (!q) {
    logger.log(MF_FATAL, "host_assoc: cannot make packet!");
    p->kill();
    return; 
  }
  memset(q->data(), 0, q->length()); 
  gnrs_req *gnrs_p = (gnrs_req *)(q->data());
  gnrs_p->type = GNRS_INSERT;
  GUID_t guid;
  guid.init(assoc_GUID); 
  //guid.init(assoc_p->client_GUID);
  memcpy(gnrs_p->GUID, guid.getGUID(), GUID_LENGTH); 
  //gnrs_p->GUID = ntohl(assoc_p->client_GUID);
  //TODO determine who should assign and report NA
  /* KN: I think the net side should do this binding
   * and create a GNRS update 
   */
  gnrs_p->net_addr = ntohl(assoc_p->host_GUID);
  gnrs_p->weight = ntohs(assoc_p->weight);
  output(0).push(q);

  p->kill();    
}


CLICK_DECLS
EXPORT_ELEMENT(MF_AssocHandler)
ELEMENT_REQUIRES(userlevel)
