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
/************************************************************************
 * MF_IntraLookUp.cc
 * This element allows the routing datagram to perform next hop look up.
 * MF_Aggregator -> MF_IntraLookUp -> MF_Segmentor
 *  Created on: Jul 16, 2011
 *      Author: Kai Su
 ************************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>
#include "mfintralookup.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_IntraLookUp::MF_IntraLookUp() 
    : _assocTable(NULL), logger(MF_Logger::init()) {
}

MF_IntraLookUp::~MF_IntraLookUp() {
}

int MF_IntraLookUp::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
                   "FORWARDING_TABLE", cpkP+cpkM, cpElement, &_forwardElem,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   "ASSOC_TABLE", cpkP, cpElement, &_assocTable, 
                   cpEnd) < 0) {
    return -1;
  }
  return 0;
}

// need to do next hop look-up based on the network address...
// set annotation at offset 0 to be the next hop GUID.
void MF_IntraLookUp::push(int port, Packet *p) {
  //check port
  if (port != 0 && port != 1) {
    logger.log(MF_FATAL,
               "intra_lkup: Packet received on unsupported port");
    exit(-1);
  }

  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)(p->data());
  uint32_t sid = ntohl(chk_trans->sid);
  uint32_t chunk_id = chk_trans->chunk_id;
  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
  if (!chunk) {
    logger.log( MF_ERROR, "intra_lkup: cannot find chunk");
    p->kill();
    return;
  }

  RoutingHeader *rheader = chunk->routingHeader();
  GUID dst_GUID = rheader->getDstGUID();

  /* chunk -- routing datagram for next hop lookup */
  if (port == 0) {        //if unicast
    
    uint32_t na_num = chunk->addressOptionsNum();
    
    if (na_num > 1){
      logger.log(MF_DEBUG, "intra_lkup(unicast): more than 1 NA is bound");
      //TODO: lookup if there is a dst NA == my NA
    } 
    if (na_num > 0) {
      Vector<NA> * addr_opts = chunk->addressOptions();
      rheader->setDstNA(addr_opts->at(0));
      logger.log(MF_DEBUG, 
                 "intra_lkup(uincast): set routing header dst NA: %u",
                 rheader->getDstNA().to_int()); 
    }
    //if na options is 0, two situations: 1. NA field is bound, 2. no record in GNRS
  } 
  //else, anycast, multicast, multihoming 

  NA dst_NA = rheader->getDstNA();
  uint32_t next_hop_GUID = 0;
  if (dst_NA.isEmpty()) {            //if no bound na
    next_hop_GUID = _forwardElem->getNextHopGUID(dst_GUID.to_int());
    logger.log(MF_DEBUG,
               "intra_lkup: No NAs are bound, chunk id: %u dst GUID: %u, "
               "guid-based routing", 
               chunk_id, dst_GUID.to_int());
  } else if (dst_NA == _myGUID) {
    uint32_t upper_protocol = ntohl(chk_trans->upper_protocol); 
    switch (upper_protocol) {
    case TRANSPORT:
      if ( _assocTable) {
        Vector<uint32_t> guids;
        uint32_t guid_cnt = _assocTable->get(dst_GUID.to_int(), guids);
        if (guid_cnt == 0) {
          logger.log(MF_DEBUG,
                     "intra_lkup: no entry in assocTable, guid-based routing");
          next_hop_GUID = _forwardElem->getNextHopGUID(dst_GUID.to_int());
        } else if (guid_cnt == 1) {
          next_hop_GUID = guids.at(0); 
          logger.log(MF_DEBUG,
                     "intra_lkup: find 1 guid %u in assoc_table, " 
                     "set as next_hop_guid",
                      next_hop_GUID);
          storeOrForward(chunk, p, next_hop_GUID, 1);  //local decision
          return; 
        } else {    //guid_cnt > 1
          /*****temp solution*************/
          next_hop_GUID = guids.at(0);
          logger.log(MF_DEBUG,
                     "intra_lkup: find %u guids in assoc table, "
                     "TEMP SOLUSTION choose first guid: %u",
                     guid_cnt, next_hop_GUID);
        }
      } else {
        next_hop_GUID = _forwardElem->getNextHopGUID(dst_GUID.to_int());
        logger.log(MF_WARN, 
                   "intra_lkup: NO assoc table!(ignore if not edge router), "
                   "dst_NA == myGUID guid: %u", dst_GUID.to_int() );
      }
      break;
    case BITRATE:
      logger.log(MF_DEBUG, "intra_lkup: received a bitrate packet"); 
      output(1).push(p);     
      return; 
    default:
      break; 
    }
  } else {  
    next_hop_GUID = _forwardElem->getNextHopGUID(dst_NA.to_int());
    logger.log(MF_DEBUG,
               "intra_lkup: NA routing, dst_NA: %u",
               dst_NA.to_int());
  }

  storeOrForward(chunk, p, next_hop_GUID); 
}

int32_t MF_IntraLookUp::storeOrForward(Chunk *chunk, Packet *p, 
                                       uint32_t next_hop_GUID, 
                                       uint32_t localFlag) {
  GUID dst_GUID = chunk->getDstGUID();
  NA dst_NA = chunk->getDstNA(); 
  uint32_t chunk_id = chunk->getChunkID();

  /*store or forward decision*/ 
  if (next_hop_GUID == 0) {
    chunk->setStatus(chunk->getStatus().set(ChunkStatus::ST_BUFFERED));
    logger.log(MF_TIME,
               "intra_lkup: No next hop dst guid: %u --> STORE chunk_id: %u",
               dst_GUID.to_int(), chunk_id);
    //TODO: make delay_msec dynamically
    _chunkManager->push_to_port(p, PORT_TO_NET_BINDER, 1000);
  } else {
    int32_t decision = 0; 
    if (localFlag == 1) {
      decision = _forwardElem->get_forward_flag(next_hop_GUID);  //next_hop_GUID is client GUID if local
    } else {
      decision = _forwardElem->get_forward_flag(dst_GUID.to_int());
      if (decision == RTG_NO_ENTRY) {
        logger.log(MF_DEBUG,
                   "intra_lkup: No entry of dst_guid: %u in routing table, "
                   "use dst_NA: %u to make store or forward decision",
                   dst_GUID.to_int(), dst_NA.to_int()); 
        decision = _forwardElem->get_forward_flag(dst_NA.to_int()); 
      }
    }
    switch (decision) {
    case RTG_DECISION_FORWARD:
      p->set_anno_u32(NXTHOP_ANNO_OFFSET, next_hop_GUID);
      logger.log(MF_TIME,
        "intra_lkup: FORWARD chunk_id: %u, dst GUID/NA %u/%u, next_hop: %u",
        chunk_id, dst_GUID.to_int(), dst_NA.to_int(), next_hop_GUID);
      output(0).push(p);
      break;
    case RTG_NO_ENTRY:
      logger.log(MF_TIME,
        "intra_lkup: RTG_NO_ENTRY");
      //same as RTG_DECISION_STORE, delay time can be greater.
    case RTG_DECISION_STORE:
      chunk->setStatus(chunk->getStatus().set(ChunkStatus::ST_BUFFERED));
      logger.log(MF_TIME,
                 "intra_lkup: STORE chunk_id: %u, dst GUID/NA %u/%u, "
                 "next_hop: %u",
                 chunk_id, dst_GUID.to_int(), dst_NA.to_int(), next_hop_GUID);
      //TODO: make delay_sec dynamically
      _chunkManager->push_to_port(p, PORT_TO_NET_BINDER, 1000);
      break;
    default:
      logger.log(MF_ERROR, 
                 "intra_lkup: NO DEFINED DECISION");
      break; 
    }
  }
}


CLICK_ENDDECLS
EXPORT_ELEMENT(MF_IntraLookUp)
ELEMENT_REQUIRES(userlevel)
