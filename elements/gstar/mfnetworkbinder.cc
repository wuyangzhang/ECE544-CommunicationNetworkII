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
 * MF_NetworkBinder.cc
 * MF_Aggregator -> MF_NetworkBinder -> MF_Segmentor
 *  Created on: Jul 16, 2011
 *      Author: Kai Su
 ************************************************************************/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>
#include <click/timestamp.hh>

#include "mfnetworkbinder.hh"

CLICK_DECLS

MF_NetworkBinder::MF_NetworkBinder():logger(MF_Logger::init()){
  _resolvingTable = new HashTable<uint32_t, Packet*> (); 
}

MF_NetworkBinder::~MF_NetworkBinder(){
}

int MF_NetworkBinder::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "RESP_CACHE", cpkP+cpkM, cpElement, &_respCache, 
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void MF_NetworkBinder::push(int port, Packet *p) {
  if (port == 0) {
    handleChunkPkt(p);
  } else if (port == 1) {
    handleGNRSResp(p);
  } else {
    logger.log(MF_ERROR, 
      "net_binder: Unrecognized packet on port %d!", port);
    p->kill();
  }
}

void MF_NetworkBinder::handleChunkPkt(Packet *p) {
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)(p->data()); 
  uint32_t chunk_id = chk_trans->chunk_id; 
  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
  if (!chunk) {
    logger.log(MF_ERROR, "net_binder: Got a invalid chunk from ChunkManger"); 
    return; 
  }
  logger.log(MF_DEBUG,
    "net_binder: Got chunk_id: %u from ChunkManager, sid: %x", 
    chunk_id, ntohl(chk_trans->sid));
  RoutingHeader *rheader = chunk->routingHeader();
  GUID dst_GUID = rheader->getDstGUID();
  gnrs_lkup_resp_t resp;
  // check if NA bound but is too old, then initiate GNRS lookup
  if (MF_Functions::passed_sec(chunk->getNABoundTime()) > 
        REBIND_NA_TIMEOUT_SEC) {
    logger.log(MF_TIME, 
      "net_binder: REBIND chunk_id: %u dst GUID: %u", 
      chunk_id, dst_GUID.to_int());
    bool ret = _respCache->get_response(dst_GUID, &resp);
    if (ret) {
      chunk->setAddressOptions(resp.NAs, resp.na_num);
      chunk->setNABoundTime(Timestamp::now_steady());
      logger.log(MF_TIME,
        "net_binder: BOUND chunk_id: %u dst GUID: %u",
        chunk_id, dst_GUID.to_int());
      output(0).push(p);   //push chunk to next element;
    } else {
      chunk->setStatus(chunk->getStatus().set(ChunkStatus::ST_RESOLVING));
      _resolvingTable->set(chunk_id, p);             //chunk chunk internal msg
      sendGNRSReqst(dst_GUID);	
      logger.log(MF_DEBUG,
                 "net_binder: Sent GNRS request, set status to ST_RESOLVING, "
                 "chunk_id: %u, status %x", 
        chunk_id, chunk->getStatus().get());
    return;
    }		
  }
    
  uint32_t num_na = chunk->addressOptionsNum();
  NA dst_NA = rheader->getDstNA(); 
  logger.log(MF_DEBUG, 
             "net_binder: Recvd chunk_id: %u dstGUID: %u dstNA: %u, "
             "# of NA options: %u",
             chunk_id, dst_GUID.to_int(), dst_NA.to_int(), num_na);

  if (num_na == 0 && dst_NA.isEmpty()) {             //hasn't been bound
    bool ret = _respCache->get_response(dst_GUID, &resp);
    if (ret) {                                       //if gnrs_resp is cached
      chunk->setAddressOptions(resp.NAs, resp.na_num);
      chunk->setNABoundTime(Timestamp::now_steady());
      logger.log(MF_TIME,
                 "net_binder: BOUND chunk_id: %u dst GUID: %u",
                 chunk_id, dst_GUID.to_int());
      output(0).push(p);                             //push chunk to next element;  
    } else { 
      chunk->setStatus(chunk->getStatus().set(ChunkStatus::ST_RESOLVING));
      _resolvingTable->set(chunk_id, p); 
      //send gnrs req
      sendGNRSReqst(dst_GUID);
      logger.log(MF_DEBUG,
                 "net_binder: Sent GNRS request and set status to ST_RESOLVING, " 
                 "chunk_id: %u, status %x", 
                  chunk_id, chunk->getStatus().get());
    }
  } else {
    /* 
     * Differentiation into gstar/policy/multi-point routing 
     * will happen at service classifier element 
     */
    output(0).push(p);
  }
}


void MF_NetworkBinder::sendGNRSReqst(GUID dst_GUID) {
  //internal gnrs_req_pkt to other element
  WritablePacket *q = Packet::make( 0, 0, sizeof(gnrs_req), 0);
  if (!q) {
    logger.log(MF_CRITICAL, 
      "net_binder: Unable to make gnrs_req pkt!");
    return;
  }
  memset(q->data(), 0, q->length());
  gnrs_req *gnrs_p = (gnrs_req *)(q->data());
  gnrs_p->type = GNRS_LOOKUP;
  memcpy(gnrs_p->GUID, dst_GUID.getGUID(), GUID_LENGTH); 
  gnrs_p->net_addr = 0;
  gnrs_p->weight = 0;
  output(1).push(q);
}


void MF_NetworkBinder::handleGNRSResp(Packet *p) {

  /* The NA of client should be the GUID of the first hop router */
  gnrs_lkup_resp_t *resp = (gnrs_lkup_resp_t *)(p->data());
  GUID guid;
  guid.init(resp->GUID);
  logger.log(MF_DEBUG, 
             "net_binder: Got GNRS resp: qry GUID: %u, num_na: %u",
             guid.to_int(), resp->na_num);
    
  /* retrieve chunks, and process them */
  Vector<Chunk*> *chk_vec = new Vector<Chunk*>();

  //if 0 chunks are found in chunkManager
  if (!_chunkManager->get(guid.to_int(), ChunkStatus::ST_RESOLVING, *chk_vec)) {
    logger.log(MF_DEBUG, "net_binder: No available chunk in ChunkManager"); 
    return;
  } else {
    logger.log(MF_DEBUG,
      "net_binder: handleGNRSResp, retrieved %u chunks", chk_vec->size());
  }

  Chunk *chunk = NULL;
  for (int i=0; i < chk_vec->size(); ++i) {
    chunk = chk_vec->at(i);
    if (chunk == NULL) {
      logger.log(MF_ERROR, 
                 "net_binder: Got a invalid chunk from ChunkManager"); 
      continue;
    }    
    
    //bind nas to chunk
    chunk->setAddressOptions(resp->NAs, resp->na_num);
    //reset the ST_RESOLVING bit in chunk status   
    chunk->setStatus(chunk->getStatus().reset(ChunkStatus::ST_RESOLVING));
    //set a new bound time; 
    chunk->setNABoundTime(Timestamp::now_steady());
    logger.log(MF_TIME,
               "net_binder: BOUND chunk_id: %u dst GUID: %u # of NAs %u",
               chunk->getChunkID(), chunk->getDstGUID().to_int(), resp->na_num);
    
    HashTable<uint32_t, Packet*>::iterator it = 
        _resolvingTable->find(chunk->getChunkID());
    if (it == _resolvingTable->end()) {
      logger.log(MF_ERROR,
                 "net_binder: missing chunk internal msg in resolving table");
      return; 
    } else {
      Packet *q = _resolvingTable->find(chunk->getChunkID()).value();
      chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)q->data(); 
      if (!q) {
        logger.log(MF_ERROR,
                   "net_binder: chunk internal msg in resolving table is invalid");
        p->kill(); 
        return; 
      }
      logger.log(MF_DEBUG, 
                 "net_binder: find chunk internal msg in resolving table, "
                 "sid: %u chunk_id: %u",
                 ntohl(chk_trans->sid), chk_trans->chunk_id);
      output(0).push(q);
      _resolvingTable->erase(it);
    }
  }
  delete chk_vec; 
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_NetworkBinder)
ELEMENT_REQUIRES(userlevel)
