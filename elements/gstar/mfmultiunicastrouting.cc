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
 * MF_MultiUnicastRouting.cc
 * This element implements multicast as multiple unicast. Duplicates of 
 * the chunk are forwarded to each destination NA with unicast routing
 * along the way.
 ************************************************************************/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>
#include "mfmultiunicastrouting.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_MultiUnicastRouting::MF_MultiUnicastRouting():logger(MF_Logger::init()) {
}

MF_MultiUnicastRouting::~MF_MultiUnicastRouting() {
}

int MF_MultiUnicastRouting::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
    "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
    "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
       //			"FORWARDING_TABLE", cpkP+cpkM, cpElement, &_forwardElem,
    cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void MF_MultiUnicastRouting::push(int port, Packet *p) {
  
  if (port != 0) {
    logger.log(MF_FATAL, "mucast_rtg: Packet received on unsupported port");
    exit(-1);
  }
 
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)(p->data()); 
  uint32_t sid = chk_trans->sid; 
  uint32_t chunk_id = chk_trans->chunk_id; 
  
  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED); 
  GUID dst_GUID = chunk->getDstGUID();
  uint32_t na_num = chunk->addressOptionsNum(); 
  Vector<NA> *bound_NAs = chunk->addressOptions(); 
  logger.log(MF_DEBUG, 
    "mucast_rtg: got chunk id: %u dst_GUID: %u, num dst_NA: %u", 
    chunk_id, dst_GUID.to_int(), na_num); 
  RoutingHeader *rheader = chunk->routingHeader();
  if (na_num == 0) {
    logger.log(MF_DEBUG,
      "mucast_rtg: no bound na"); 
  } else if (na_num == 1) {
    rheader->setServiceID(0);      //set service_id to unicast
    rheader->setDstNA(bound_NAs->at(0));
    logger.log(MF_DEBUG,
      "mucast_rtg: has 1 bound na, foward on"); 
    output(0).push(p); 
  } else {
    for (int i = 0; i< na_num; ++i) {
      if (i != (na_num - 1)) {
        Chunk *dup_chunk = _chunkManager->alloc_dup(chunk);
        logger.log(MF_DEBUG,
          "mucast_rtg: duplicate chunk with chunk_id: %u, new chunk_id %u",
          chunk->getChunkID(), dup_chunk->getChunkID()); 
        WritablePacket *dp = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
        if (!dp) {
          logger.log(MF_ERROR,
            "mucast_rtg: Failed to make chunk internal message for chunk_id %u",
            dup_chunk->getChunkID());
          continue; 
        }
        memset( dp, 0, sizeof(chk_internal_trans_t));
        chk_internal_trans_t *dup_chk_trans = (chk_internal_trans_t*)(dp->data());
        dup_chk_trans->sid = 0;
        dup_chk_trans->chunk_id = dup_chunk->getChunkID();  
        output(0).push(dp); 
      } else {
        output(0).push(p); 
      }
    }
  }
}	

CLICK_DECLS
EXPORT_ELEMENT(MF_MultiUnicastRouting)
ELEMENT_REQUIRES(userlevel)
