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
 * MF_Segmentor.hh
 *
 *  Created on: Jun 2, 2011
 *      Author: Kai Su
 */

#ifndef MF_SEGMENTOR_HH_
#define MF_SEGMENTOR_HH_

#include <click/element.hh>
#include <click/timer.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/task.hh>

#include <pthread.h>

#include "mf.hh"
#include "mfcsyntimer.hh"
#include "mfchunkmanager.hh"
#include "mfrouterstats.hh"
#include "mflogger.hh"

CLICK_DECLS
class HandlerCall;

/**
 * In Ports: 0 - Chunk packet; 1 - CSYN-ACK packet
 * Out Ports: 0 - Data packet, CSYN packet ; 1 - Chunk packet for rebinding
 */

//<local_hop_id, chunk*>
typedef HashTable<uint32_t, Chunk*> ReadyChunkMap;

typedef struct {
  Chunk *chunk;
  bool csyn_stat; 
} in_processing_chunk_t;

//local_hop_id, <chunk*, csyn_stat>
typedef HashTable<uint32_t, in_processing_chunk_t> InProcessingChunkMap; 
//<guid, chunk*>
typedef HashTable<uint32_t, Chunk*> WaitingChunkMap; 


class MF_Segmentor : public Element {
 public:
  MF_Segmentor();
  ~MF_Segmentor();

  const char *class_name() const		{ return "MF_Segmentor"; }
  const char *port_count() const		{ return "2/2"; }
  const char *processing() const		{ return "h/h"; }

  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p);

  int initialize(ErrorHandler *);
  bool run_task(Task *); 
  
  static void handleCSYNExpiry(Timer *, void *);
  void CSYNexpire(uint32_t, uint32_t, uint32_t);
  void add_handlers();

 private:
  static String read_handler(Element *e, void *thunk);

  void handleChkPkt(Packet *);
  void handleACKPkt(Packet *);
  void sendCSYN(uint32_t, int, bool);
  
  //parse lost seq no from CSYN-ACK
  Vector<int>* getLostSeq(char *bitmap, uint32_t chunksize);
  
  //thread safe hopID++
  inline uint32_t getNewLocalHopID() {
    pthread_mutex_lock(&_hopID_lock);
    ++_mono_hop_ID;
    pthread_mutex_unlock(&_hopID_lock);
    return _mono_hop_ID; 
  }

  //thread sage window size increase or decrease
  inline void enlargeCurWindowSize() {
    pthread_mutex_lock(&_window_lock);
    ++_cur_window;
    pthread_mutex_unlock(&_window_lock); 
  }
  
  inline void shrinkCurWindowSize() {
    pthread_mutex_lock(&_window_lock);
    --_cur_window; 
    pthread_mutex_unlock(&_window_lock); 
  }

  //chech whether SendingPool is full
  inline bool isSendingPoolFull() {
    pthread_mutex_lock(&_window_lock);
    bool ret = (_cur_window >= _window_size && _cur_window >=0) ? true : false; 
    pthread_mutex_unlock(&_window_lock);
    return ret; 
  }
  
  //find smallest ready local hop id from ReadyCache  
  uint32_t findMinReadyHopID();

  //if needs to send to NetBinder for rebinding, 
  //release chunks from sendingPool, kill csyn timer
  void release(uint32_t); 
  
  //This Task keeps check if there are available chunks for sending out
  Task _task;
  
  //chunks that are sending
  InProcessingChunkMap *_sendingPool;
  pthread_mutex_t _sending_lock; 
  //complete chunks, ready to send out
  ReadyChunkMap *_readyCache;
  pthread_mutex_t _ready_lock;
  //imcomplete chunks, waiting pkts 
  WaitingChunkMap *_waitingCache; 
  pthread_mutex_t _waiting_lock;
  
/* a hash map to show whether a chunk's csyn has been acked */
  //TODO remove this table, can be move to chunk's meta data
  //HashTable<uint32_t, int> * _isAcked;  
  
  //cur window size, increase if adds a chunk to SendingPool
  //decrease if removes a chunk from SendingPool
  volatile int _cur_window;
  pthread_mutex_t _window_lock; 

  //Monotonically increasing final hop ID assigned 
  //to outgoing chunks 
  uint32_t _mono_hop_ID;
  pthread_mutex_t _hopID_lock;  
  
  int _sent_chk_count;
  
  //size of SendingPool
  uint32_t _window_size;
  
  //for update router's readyCache size and WaitingCache size
  MF_RouterStats *_routerStats;

  typedef HashTable<uint32_t, Packet*> DgramTable;
  DgramTable *_dgramTable;
       
  MF_ChunkManager *_chunkManager; 
  MF_Logger logger;
};


CLICK_ENDDECLS;

#endif /* MF_SEGMENTOR_HH_ */
