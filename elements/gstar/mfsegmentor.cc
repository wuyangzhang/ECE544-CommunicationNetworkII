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
/*************************************************************************
 * MF_Segmentor.cc
 * Segmentor is the last element of MobilityFirst click router(except Queue
 * and ToDevice. It receives routing layer datagram from previous element 
 * within this router, sends out CSYN and data packets, and also receives ACK.
 * Created on: Jun 2, 2011
 *      Author: Kai Su
 *************************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <click/vector.hh>
#include <clicknet/ether.h>

#include "mfsegmentor.hh"
#include "mffunctions.hh"

CLICK_DECLS

MF_Segmentor::MF_Segmentor() : _task(this), _cur_window(0), _sent_chk_count(0),
                              _window_size(DEFAULT_WINDOW_SIZE), 
                              _mono_hop_ID(0),logger(MF_Logger::init()) {
  _sendingPool = new InProcessingChunkMap ();
  _readyCache = new ReadyChunkMap (); 
  _waitingCache = new WaitingChunkMap ();

  //_isAcked = new HashTable<uint32_t, int>;
  
  _dgramTable = new DgramTable;

  pthread_mutex_init(&_hopID_lock, NULL);
  pthread_mutex_init(&_window_lock, NULL); 
  
  pthread_mutex_init(&_ready_lock, NULL);
  pthread_mutex_init(&_waiting_lock, NULL);
  pthread_mutex_init(&_sending_lock, NULL); 
}

MF_Segmentor::~MF_Segmentor() {
}

int MF_Segmentor::configure(Vector<String> &conf, ErrorHandler *errh){
  if(cp_va_kparse(conf, this, errh,
                  "ROUTER_STATS", cpkP+cpkM, cpElement, &_routerStats,
                  "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                  "WINDOW_SIZE", cpkP, cpUnsigned, &_window_size,
                  cpEnd) < 0) {
    return -1;
  }
  return 0;
}

int MF_Segmentor::initialize(ErrorHandler *errh) {
  logger.log(MF_INFO, "seg: window size: %u", _window_size); 
  ScheduleInfo::initialize_task(this, &_task, true, errh); 
}

bool MF_Segmentor::run_task(Task *) {
  
  //if mutex is not locked by other thread, lock successfully
  if (pthread_mutex_trylock(&_ready_lock) == 0) {
    //if sending pool is not full, and there are chunks is ready Q
    while ((!isSendingPoolFull()) && (_readyCache->size() > 0)) {
      uint32_t hop_ID = findMinReadyHopID();
      logger.log(MF_DEBUG,
                 "seg: starting trxfr hop ID: %d; ready chks: %d",
                 hop_ID, _readyCache->size());
      //find ready chunks, erase from readyQ
      ReadyChunkMap::iterator it = _readyCache->find(hop_ID); 
      Chunk *readyChunk = it.value();
      //erase entry from readyCache
      _readyCache->erase(it);
 
      //current window size ++
      enlargeCurWindowSize();
      
      in_processing_chunk_t ipchunk;
      ipchunk.chunk = readyChunk;
      ipchunk.csyn_stat = false; 
      //push chunk into sending pool
      pthread_mutex_lock(&_sending_lock);
      //_sendingPool->set(hop_ID, readyChunk);
      _sendingPool->set(hop_ID, ipchunk); 
      pthread_mutex_unlock(&_sending_lock);
      uint32_t chk_pkt_count = readyChunk->getChkPktCnt();
      //send CSYN first time
      sendCSYN(hop_ID, chk_pkt_count, false);
    }
    //if no chunks in readyQ or sending pool is full
    pthread_mutex_unlock(&_ready_lock);
  }

  //check waitingQ to see if there is complete chunk
  Chunk *waitingChunk = NULL;
  if (pthread_mutex_trylock(&_waiting_lock) == 0) { 
    WaitingChunkMap::iterator it = _waitingCache->begin(); 
    while (it != _waitingCache->end()) {
      waitingChunk = it.value();
      if (waitingChunk->getStatus().isComplete()) {
// && 
//              waitingChunk->getStatus().isAcked()) {
        uint32_t local_hop_ID = getNewLocalHopID(); 
        pthread_mutex_lock(&_ready_lock);
        _readyCache->set(local_hop_ID, waitingChunk);
        pthread_mutex_unlock(&_ready_lock); 
        logger.log(MF_TIME,
                   "seg: push chunk_id: %u in waitingCache to readyCache "
                   "hop_ID: %u",
                   waitingChunk->getChunkID(), local_hop_ID);
        //remov entry from waitingCache
        it = _waitingCache->erase(it);
      } else {
        ++it;
      }
    }
    pthread_mutex_unlock(&_waiting_lock);
    _routerStats->setNumWaitingChunk(_waitingCache->size());
  }
  _routerStats->setNumReadyChunk(_readyCache->size());
  _task.reschedule(); 
}

void MF_Segmentor::handleChkPkt(Packet *p){
  
  uint32_t next_hop_GUID = p->anno_u32(NXTHOP_ANNO_OFFSET);
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)(p->data()); 
  uint32_t chunk_id = chk_trans->chunk_id;

  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
  if (!chunk) {
    logger.log(MF_ERROR,
               "seg: handleChkPkt: Cannot find chunk id: %u in ChunkManager", 
               chunk_id);
    return; 
  }
  
  uint32_t pkt_cnt = chunk->getChkPktCnt();
  
  logger.log(MF_DEBUG,
             "seg: Got chunk_id: %u, next_hop_guid: %u, num_pkts: %u",
             chunk_id, next_hop_GUID, pkt_cnt); 
  chunk->setNextHopGUID(next_hop_GUID);
  
  _dgramTable->set(chunk_id, p);
  
  //if chunk is incomplete, push to waitingCache
  if (!chunk->getStatus().isComplete()) {
    logger.log(MF_DEBUG,
               "seg: Push chunk with chunk_id %u into waitingCache", 
               chunk_id);
    pthread_mutex_lock(&_waiting_lock);
    _waitingCache->set(chunk_id, chunk);
    pthread_mutex_unlock(&_waiting_lock);
    _routerStats->setNumWaitingChunk(_waitingCache->size());
  } else if (!isSendingPoolFull()) {  //if chunk is complete, sending pool is not full
    //push to sending pool and send it directly
    uint32_t local_hop_ID = getNewLocalHopID(); 
    
    logger.log(MF_DEBUG, 
               "seg: SenginPool is not full,"
               "Push chunk with chunk_id %u into SendingPool",
               chunk_id);
    in_processing_chunk_t ipchunk; 
    ipchunk.chunk = chunk;
    ipchunk.csyn_stat = false; 
    pthread_mutex_lock(&_sending_lock);
    _sendingPool->set(local_hop_ID, ipchunk);
    pthread_mutex_unlock(&_sending_lock);
    //increase window size
    enlargeCurWindowSize(); 
    uint32_t chk_pkt_count = chunk->getChkPktCnt();
    sendCSYN(local_hop_ID, chk_pkt_count, false);
  } else {  //if chunk is complete, and sending pool is full
    uint32_t local_hop_ID = getNewLocalHopID(); 
    //push chunk to ready Q
    pthread_mutex_lock(&_ready_lock); 
    _readyCache->set(local_hop_ID, chunk);
    pthread_mutex_unlock(&_ready_lock);

    _routerStats->setNumReadyChunk(_readyCache->size());
    logger.log(MF_DEBUG,
               "seg: SendingPool is full, "
               "Push chunk with chunk_id %u into readyCache", 
               chunk_id); 
  }
}
	
void MF_Segmentor::handleACKPkt(Packet *p){
  csyn_ack_t * ack = (csyn_ack_t *)p->data();
  uint32_t hopID = ntohl(ack->hop_ID);
  uint32_t chk_pkt_count = ntohl(ack->chk_pkt_count);

  Chunk *chunk = NULL;
  pthread_mutex_lock(&_sending_lock); 
  InProcessingChunkMap::iterator it = _sendingPool->find(hopID);
  if (it == _sendingPool->end()) {
    logger.log(MF_ERROR, 
               "seg: Got a csyn_ack, hopID: %u, chunk is gone "
               "from sending pool on trxfr complete! Discard",
               hopID);
    if (_sendingPool->size() == 0) {
      logger.log(MF_ERROR,
                 "seg: sending pool empty on trxfr complete");
    }
    pthread_mutex_unlock(&_sending_lock); 
    p->kill();
    return; 
  } else {
    chunk = it.value().chunk;
    pthread_mutex_unlock(&_sending_lock); 
    if (!chunk) {
      logger.log(MF_ERROR, 
                 "seg: Got a invalid chunk from sending pool");
      p->kill();
      return; 
    }
  }

  uint32_t chunk_id = chunk->getChunkID(); 
  uint32_t next_hop_GUID = chunk->getNextHopGUID().to_int();
  
  logger.log(MF_DEBUG, 
             "seg: Find chunk_id: %u next_hop_GUID: %u in Sending Pool", 
             chunk_id, next_hop_GUID); 
  
  /* set the csyn acked flag */
  //_isAcked->set(hopID, 1);
  //chunk->setStatus(chunk->getStatus().setCSYNAckReceived()); 
  it.value().csyn_stat = true; 	
  /* unschedule the timer */
  if (chunk->csynTimer()->isAlive()) {
    logger.log(MF_DEBUG, 
               "seg: CSYN timer is alive, unschedule it");
    // if timer is scheduled, unschedule it
    chunk->csynTimer()->unschedule(); 
  } else {
    // if got a timeout CSYN-ACK, return
    logger.log(MF_WARN,
               "seg: received a timed out CSYN-ACK");
    p->kill();
    return; 
  }

  Vector<int> *lost = getLostSeq(ack->bitmap, chk_pkt_count);
  uint32_t lost_size = lost->size();
  uint32_t rcvd_size = chk_pkt_count - lost_size; 
  if (lost_size) {
    logger.log(MF_INFO, 
               "seg: Got CSYN-ACK for hopID: %u chunk_id: %u "
               "rcvd/**lost**/total: %u/**%u**/%u", 
                hopID, chunk_id, rcvd_size, lost_size, chk_pkt_count);
  } else {
    logger.log(MF_DEBUG, 
               "seg: Got CSYN-ACK for hopID: %u chunk_id: %u "
               "rcvd/**lost**/total: %u/**%u**/%u", 
               hopID, chunk_id, rcvd_size, lost_size, chk_pkt_count);
  }
  
  if (lost_size) { //if some pkts are lost
	logger.log(MF_DEBUG, "There were %d packets lost", lost_size);
    Packet *_pkt = NULL;
    //send chunk 
    Vector<int>::iterator it_end = lost->end(); 
    for (Vector<int>::iterator it = lost->begin(); it != it_end; ++it) {
      _pkt = chunk->allPkts()->at(*it);
      assert(_pkt);
      _pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, next_hop_GUID);
      //store cloned for retransmission
      Packet *p_clone = _pkt->clone();
      if (!p_clone) {
        logger.log(MF_ERROR, "seg: Cannot make cloned packet"); 
        break; 
      }
      hop_data_t * cur_pkt = (hop_data_t *)(p_clone->data());
      //update hop ID
      cur_pkt->hop_ID = htonl(hopID);
      _pkt = NULL;
      output(0).push(p_clone);    //push to next element for sending out
  	logger.log(MF_DEBUG, "Pushed packet %d", cur_pkt->seq_num);
    }
    //send CSYN
    sendCSYN(hopID, chk_pkt_count, false);
  } else {    //if all pkts are received
    _sent_chk_count++;
    logger.log(MF_TIME, 
               "seg: SENT chunk_id: %u hop ID: %u to dst guid: %u", 
               chunk_id, hopID, next_hop_GUID);
    //remove chunk from sending pool
    logger.log(MF_DEBUG, "seg: Remove hopID: %u from sendingPool", hopID);
    //_sendingPool->erase(hopID);
    release(hopID); 
    //change window size

    /* free memory */
    //delete chunk from ChunkManager
    //_chunkManager->remove(chunk->getChunkID());
    _chunkManager->removeData(chunk); 
    //delete internal chunk transfer msg
    _dgramTable->erase(chunk_id);
  }
  delete lost; 
  p->kill();
}


inline void MF_Segmentor::push(int port, Packet *p) {
  if (port==0) {
    handleChkPkt(p);
  } else if (port==1) {
    handleACKPkt(p);
  } else {
    logger.log(MF_FATAL, "seg: Packet recvd on unsupported port");
    exit(-1);
  }		
}

/* release chunk for rebinding */
void MF_Segmentor::release(uint32_t hop_ID) {

  /* manipulate sending pool atomically */
  pthread_mutex_lock(&_sending_lock);
  InProcessingChunkMap::iterator it = _sendingPool->find(hop_ID);
  if (it != _sendingPool->end()) {
    Chunk *chunk = it.value().chunk;
    chunk->csynTimer()->kill();
    _sendingPool->erase(it); 
  } else {
    logger.log(MF_DEBUG,
               "seg: fail to release chunk"); 
  }
  pthread_mutex_unlock(&_sending_lock);
  //_cur_size--
  shrinkCurWindowSize();
}

/** 
 * getLostSeq() takes as input the bitmap from ACK message 
 * and return the indices of
 * lost and received packets.
 **/
Vector<int>* MF_Segmentor::getLostSeq (char *bitmap, uint32_t chunksize) {
  Vector<int> *lost = new Vector<int>(); 
  for(unsigned i=0; i<chunksize; i++) {
    if ((bitmap[(i)/8] & (0x01 << ((i)%8)))==0) {
      lost->push_back(i);
    }
  }
  return lost;
}

void MF_Segmentor::sendCSYN(uint32_t hop_ID, int chk_size, bool is_resend) {
  InProcessingChunkMap::iterator it = _sendingPool->find(hop_ID);
  Chunk *chunk = NULL;
  if (it != _sendingPool->end()) {
    chunk = it.value().chunk;
    if (!chunk) {
      logger.log(MF_ERROR,
                 "seg: Got an invalid chunk in sending pool"); 
      return;
    }
  } else {
    logger.log(MF_ERROR,
               "seg: Cannot find local hop ID: %u in sending pool",
               hop_ID);
    return; 
  }
 
  uint32_t chunk_id = chunk->getChunkID();
  uint32_t next_hop_GUID = chunk->getNextHopGUID().to_int();
 
  // TODO KN: no need to abort if making good progress
  uint32_t passed_NA_sec = MF_Functions::passed_sec(chunk->getNABoundTime()); 
  // check if NA needs rebinding
  if (passed_NA_sec > REBIND_NA_TIMEOUT_SEC) {
    DgramTable::iterator it = _dgramTable->find(chunk_id);
    assert(it != _dgramTable->end());
    Packet *_pkt = it.value();
    // push the chunk to NetBinder, and cleanup memory 
    // for this chunk at Segmentor 
    logger.log(MF_INFO, 
               "seg: Chunk passed NA rebind threshold. "
               "chunk_id: %u duration: %u sec", 
               chunk_id, passed_NA_sec);
    /* release chunk for rebind */
    release(hop_ID);
    
    //push to net_binder
    output(1).push(_pkt);             
    return;
  }

  // check if next hop needs rebinding
  uint32_t passed_nxthop_sec = 
      MF_Functions::passed_sec(chunk->getNextHopBoundTime()); 
  if (passed_nxthop_sec > REBIND_NXTHOP_TIMEOUT_SEC) {
    DgramTable::iterator it = _dgramTable->find(chunk_id);
    assert(it != _dgramTable->end());
    Packet *_pkt = it.value();
    // send chunk for re-routing
    // TODO: KN: this is currently conflated with NA rebind as
    // it is output on the same port
    logger.log(MF_INFO, 
               "seg: Chunk passed nxthop rebind threshold. "
               "chunk_id: %u duration: %u sec", 
               chunk_id, passed_nxthop_sec);
    /* release chunk for rebind */
    release(hop_ID);
    
    //push to net_binder 
    output(1).push(_pkt);
    return;
  }
  
  WritablePacket * csyn_pkt = Packet::make(sizeof(click_ether) + sizeof(click_ip), 
                                           0, CSYN_SIZE, 0);
  if (!csyn_pkt) {
    logger.log(MF_CRITICAL, "seg: cannot make csyn packet");
    return;
  }
  
  memset(csyn_pkt->data(), 0, csyn_pkt->length());
  csyn_t * csyn = (csyn_t *)csyn_pkt->data();
  csyn->type = htonl(CSYN_PKT);
  csyn->hop_ID = htonl(hop_ID);
  csyn->chk_pkt_count = htonl(chk_size);
  csyn_pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, next_hop_GUID);

  it.value().csyn_stat = false; 
  // create timer if just starting on this chunk
  if (!is_resend) {
    chunk->csynTimer()->init(&MF_Segmentor::handleCSYNExpiry, this, hop_ID, 
                             chk_size, chunk_id); 
  } else {
    chunk->csynTimer()->reschedule(); 
  }

  logger.log(MF_DEBUG, 
             "seg: sent CSYN hop id: %u, chunk_id: %u, nexthop: %u, "
             "size: %u pkts", 
             hop_ID, chunk_id, next_hop_GUID, chk_size);
  output(0).push(csyn_pkt);
}

/**
 * Every time a new chunk is to be sent, findID() is called to get the 
 * the smallest available hopID.
 **/
uint32_t MF_Segmentor::findMinReadyHopID() {
  uint32_t id_to_send = _readyCache->begin().key();
  ReadyChunkMap::iterator it_end = _readyCache->end(); 
  for (ReadyChunkMap::iterator it = _readyCache->begin(); it != it_end; ++it) {
    if (id_to_send > it.key()) {
      id_to_send = it.key(); 
    }
  }
  logger.log(MF_DEBUG, 
             "seg: find ready local hop_ID, smallest=%d\n", 
             id_to_send); 
  return id_to_send;
}

void MF_Segmentor::handleCSYNExpiry(Timer *, void *data) {
  CSYNTimerData *td = (CSYNTimerData *)data;
  assert(td);
  td->elem->CSYNexpire(td->hop_ID, td->chk_pkt_count, td->chk_id);
}

void MF_Segmentor::CSYNexpire(uint32_t hop_ID, uint32_t chk_size, 
                              uint32_t chk_id) {
  
  logger.log(MF_INFO, 
             "seg: Timer expired for unACKed chk_id: %u hop id: %u ; " 
             "resending CSYN", 
             chk_id, hop_ID);
  sendCSYN(hop_ID, chk_size, true);
}

String MF_Segmentor::read_handler(Element *e, void *thunk) {
  MF_Segmentor *seg = (MF_Segmentor *)e;
  switch ((intptr_t)thunk) {
  case 1:
    return String(seg->_sent_chk_count);
  default:
    return "<error>";
  }
}

void MF_Segmentor::add_handlers() {
  add_read_handler("sent_chk_count", read_handler, (void *)1);
}

/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<hop_data_t *>;
template class HashMap<int, Vector<hop_data_t *> >;
#endif
*/

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_Segmentor)
ELEMENT_REQUIRES(userlevel)
