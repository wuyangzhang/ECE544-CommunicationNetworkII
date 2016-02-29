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
/***********************************************************************
* MF_Aggregator.cc
* aggregate individual packet into chunk of packets; send ack to previous
* node after receiving CSYN
*  Created on: Jun 2, 2011
*      Author: Kai Su
* MF Router use hop_ID to differentiate different flows. 
************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/standard/scheduleinfo.hh>
#include <click/vector.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <click/etheraddress.hh>

#include "mfaggregator.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_Aggregator::MF_Aggregator() 
    : _timeToAck(0), _recvd_chk_count(0), logger(MF_Logger::init()) {
  _hopIDMap = new HopIDMap();
}

MF_Aggregator::~MF_Aggregator() {
}

int MF_Aggregator::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                  "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
                  "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                  "ROUTER_STATS", cpkP+cpkM, cpElement, &_routerStats, 
                  cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void MF_Aggregator::push(int port, Packet *p) {
  // CSYN packet
  if (port == 1) {
    handleCSYNPkt(p);    
  // data packet
  } else if(port == 0) {
    handleDataPkt(p); 
  } else {
    logger.log(MF_FATAL, "agg: Packet recvd on unsupported port");
    exit(-1); 
  }
}

void MF_Aggregator::handleCSYNPkt (Packet *p) {
  //TODO: host key doesn't have to be a str. A 4byte integer hash may suffice
  //String src_guid_str(src_GUID);

  uint32_t src_id = p->anno_u32(SRC_GUID_ANNO_OFFSET);
  
  csyn_t *csyn = (csyn_t *)p->data();
  uint32_t remote_hop_ID = ntohl(csyn->hop_ID);
  uint32_t chk_pkt_count = ntohl(csyn->chk_pkt_count);

  logger.log(MF_DEBUG,
    "agg: CSYN from GUID: %u, remote hopID: %u, # of packets %u",
    src_id, remote_hop_ID, chk_pkt_count);

  if (_routerStats->isBusy()) {
    logger.log(MF_WARN, 
      "agg: BUSY! # of waiting chunks: %u, # of ready chunks: %u", 
      _routerStats->getNumWaitingChunk(), _routerStats->getNumReadyChunk());
    return; 
  }

  //if this is the first CSYN for this chunk, initialize hopIDMap for this src guid
  if (_hopIDMap->find(src_id) == _hopIDMap->end()) {
    _hopIDMap->find_insert(src_id, new HashTable<uint32_t, Chunk*>());
  }

  Chunk *chunk = NULL;
  if ((*_hopIDMap)[src_id]->find(remote_hop_ID) == (*_hopIDMap)[src_id]->end()) {
    //chunk manager allocates a new chunk 
    chunk = _chunkManager->alloc(chk_pkt_count);
    if (!chunk) {
      logger.log(MF_DEBUG, "agg: Failed to allocate a new chunk");
      p->kill();
      return;
    }
    //insert new element to hopIDMap
    (*_hopIDMap)[src_id]->set(remote_hop_ID, chunk);

  } else {
    chunk = (*_hopIDMap)[src_id]->find(remote_hop_ID).value();
  }

  /* 
   * Send ack for this csyn. The hop ID in ack 
   * is derived from the received csyn.
   * that is, ack->hop_ID != local hop ID
   */
  // Hang a byte array at end of ack hdr with a terminating null
  uint32_t ack_size = sizeof(csyn_ack_t) + chk_pkt_count/8 + 1;
  WritablePacket *ack_pkt = Packet::make(sizeof(click_ether) + sizeof(click_ip),
                                         0, ack_size, 0);
  if (!ack_pkt) {
    logger.log(MF_CRITICAL, "agg: Unable to make csyn-ack pkt!");
    p->kill();
    return;
  }
  memset(ack_pkt->data(), 0, ack_size);
  csyn_ack_t *csyn_ack = (csyn_ack_t *)ack_pkt->data();
  csyn_ack->type = htonl(CSYN_ACK_PKT);
  csyn_ack->hop_ID = htonl(remote_hop_ID);
  csyn_ack->chk_pkt_count = htonl(chk_pkt_count);
  //TODO: KN: this needs be copied out in NBO
  memcpy(csyn_ack->bitmap, chunk->ackBits(), chk_pkt_count/8 + 1);
  /* set next hop GUID anno. for eth encap & port determination */
  ack_pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, src_id);

  output(1).push(ack_pkt);                         //push ack packet to outQ_ctrl
  
  uint32_t acked_pkt_cnt = chunk->getChkPktCnt();

  if (chunk->getStatus().isComplete()) {
    //setAcked is a temp solution in order to avoid deleting chunk too early
    //TODO: meta data is keep when chunkManager do remove operation
    //chunk->setStatus(chunk->getStatus().setAcked()); 
    //logger.log(MF_DEBUG,
    //  "agg: set chunk status to ST_ACKED, chunk_id: %u, status: %x",
    //  chunk->getChunkID(), chunk->getStatus().get());
    _hopIDMap->find(src_id).value()->erase(remote_hop_ID);    //remove entry from table;
  }
  logger.log(MF_DEBUG,
             "agg: CSYN-ACK sent to src guid: %u, chunk_id: %u "
             " (acks %u/%u pkts)",
             src_id, chunk->getChunkID(), acked_pkt_cnt, chk_pkt_count);
  p->kill(); 
}

void MF_Aggregator::handleDataPkt (Packet *p) {
  uint32_t src_id = p->anno_u32(SRC_GUID_ANNO_OFFSET);
  
  hop_data_t * cur_pkt = (hop_data_t *)(p->data());
  uint32_t seq_num = ntohl(cur_pkt->seq_num);
  uint32_t remote_hop_ID = ntohl(cur_pkt->hop_ID);

  /** 
    * Ensure that data pkts were preceded by signal pkts 
    * that should have set up appropriate meta state 
    */
  if (!_hopIDMap->find(src_id)) {
    /* no state set up for this src */
    click_chatter("ERROR: agg: No local src ref found "
                  "for data pkt, src guid: %u. Discarding.",
                  src_id);
    p->kill();
    return;
  } else if (!(*_hopIDMap)[src_id]->find(remote_hop_ID)) {
    /* no state set up for this chunk */
    click_chatter("ERROR: agg: No local chk ref found "
                  "for data pkt, src guid %u, "
                  "remote hopID %u. Discarding.",
                   src_id, remote_hop_ID);
    p->kill();
    return;
  } else {
    Chunk *chunk = (*_hopIDMap)[src_id]->find(remote_hop_ID).value();
    if (!chunk) {                             
      logger.log(MF_ERROR,
                 "agg: cannot find valid chunk_id: %u. Discarding",
                 chunk->getChunkID());
      p->kill();
      return;
    } else { 
      uint32_t ret = chunk->insert(seq_num, p);
      if (ret < 0) {
        logger.log(MF_ERROR, "agg: pkt cannot insert to the chunk");
      }

      if (chunk->getStatus().isInitialized()) {
//        logger.log(MF_DEBUG, "agg: Chunk is initialized");
        if(chunk->getDstGUID() == _myGUID && (chunk->getUpperProtocol() == VIRTUAL_CTRL || chunk->getUpperProtocol() == VIRTUAL_DATA || chunk->getUpperProtocol() == VIRTUAL_ASP)) {
            logger.log(MF_DEBUG, "agg: RECEIVED a message for upper protocol %u", chunk->getUpperProtocol());
            if(chunk->getStatus().isComplete()) {
            logger.log(MF_TIME, "agg: RECEIVED Full Chunk! chunk_id: %u, src guid: %u, dst guid: %u status: %x",
                     chunk->getChunkID(), chunk->getSrcGUID().to_int(),
                     chunk->getDstGUID().to_int(), chunk->getStatus().get());

            _chunkManager->addToDstGUIDTable(chunk->getChunkID());

            WritablePacket *pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t),
                                           0);
            if (!pkt){
            logger.log(MF_CRITICAL, 
                     "agg: cannot make internal chunk msg packet!");
            return;
            }
            memset(pkt->data(), 0, pkt->length());
            //fill internal chunk msg
            chk_internal_trans_t* chk_trans = (chk_internal_trans_t*)(pkt->data());
            chk_trans->sid = htonl(chunk->getServiceID().to_int());
            chk_trans->chunk_id = chunk->getChunkID();
            chk_trans->upper_protocol = htonl(chunk->getUpperProtocol()); 
            _recvd_chk_count++;

            output(2).push(pkt);
            }
            return; 
        }

        else if (seq_num == 0) {
            _chunkManager->addToDstGUIDTable(chunk->getChunkID());
            logger.log(MF_TIME, "agg: RECEIVED First Packet, chunk_id: %u", chunk->getChunkID());

            WritablePacket *pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t),
                                           0);
            if (!pkt){
            logger.log(MF_CRITICAL, "agg: cannot make internal chunk msg packet!");
            return;
            }
            memset(pkt->data(), 0, pkt->length());
            //fill internal chunk msg
            chk_internal_trans_t* chk_trans = (chk_internal_trans_t*)(pkt->data());
            chk_trans->sid = htonl(chunk->getServiceID().to_int());
            chk_trans->chunk_id = chunk->getChunkID();
            chk_trans->upper_protocol = htonl(chunk->getUpperProtocol()); 
            _recvd_chk_count++;

        //check if the chunk has only one packet
        //check at the end of this function may asscess deleted chunk
            if (chunk->getStatus().isComplete()) {
                logger.log(MF_TIME,
                     "agg: RECEIVED Full Chunk! chunk_id: %u, src guid: %u, "
                     "dst guid: %u status: %x",
                     chunk->getChunkID(), chunk->getSrcGUID().to_int(),
                     chunk->getDstGUID().to_int(), chunk->getStatus().get());

            //if it is a upper protocol msg for this router
            //TODO: this part should not be in agg, agg makes no routing decision
                if (chunk->getDstGUID() == _myGUID && chunk->getUpperProtocol() == BITRATE) {
                    logger.log(MF_DEBUG,
                       "agg: RECEIVED a message for upper protocol %u", chunk->getUpperProtocol());

            //push to upper protocol classifier
                    output(2).push(pkt);
                    return; 
                }
            }
        //push chunk internal msg to net_binder
            output(0).push(pkt);
            return; 
        }
      } else if (seq_num == 0 && !chunk->getStatus().isInitialized()) {
        //inserted first packet but chunk is uninitialized, remove the chunk
        assert(false);
        return; 
      }

      if (chunk->getStatus().isComplete()) {
        logger.log(MF_TIME,
                   "agg: RECEIVED Full Chunk! src guid: %u, chunk_id: %u, "
                   "status: %x seq_num: %u",
                    src_id, chunk->getChunkID(), chunk->getStatus().get(), seq_num);
      }
    }
  }
}

String MF_Aggregator::read_handler(Element *e, void *thunk){

  MF_Aggregator *agg = (MF_Aggregator *)e;
  switch ((intptr_t)thunk) {
  case 1:
    return String(agg->_recvd_chk_count);
  default:
    return "<error>";
  }
}

void MF_Aggregator::add_handlers() {

  add_read_handler("recvd_chk_count", read_handler, (void *)1);
}

/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<hop_data_t *>;
template class HashMap<int, Vector<hop_data_t *> >;
#endif
*/

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_Aggregator)
ELEMENT_REQUIRES(userlevel)
