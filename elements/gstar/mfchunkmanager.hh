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

#ifndef MF_CHUNKMANAGER_HH_
#define MF_CHUNKMANAGER_HH_

#include <click/element.hh>
#include <click/timestamp.hh>
#include <click/sync.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>
#include <click/task.hh>
#include <click/timer.hh>
#include <click/string.hh>
#include <click/glue.hh>

#include <pthread.h>

#include "mf.hh"
#include "mflogger.hh"
#include "mfchunk.hh"
#include "mfchunkstatus.hh"

#define CACHE_TIMEOUT 0
#define PORT_TO_NET_BINDER 0
#define PORT_TO_SEG 1

#define DEFAULT_CLEANUP_THRESHHOLD 1000
#define DEFAULT_CLEANUP_PERCENTAGE  90  //cleanup 90% removable chunks everytime`
#define DEFAULT_CLEANUP_PERIOD_SEC 60

CLICK_DECLS

class Chunk; 

class MF_ChunkManager : public Element {
 public:
  MF_ChunkManager();
  ~MF_ChunkManager();

  const char *class_name() const	{ return "MF_ChunkManager"; }
  const char *port_count() const	{ return "0/0-3"; }
  const char *processing() const	{ return AGNOSTIC; }
 
  int initialize(ErrorHandler *);  
  bool run_task(Task *); 
  
  int configure(Vector<String> &, ErrorHandler *);

  Chunk* alloc (uint32_t pkt_cnt);
  
  Chunk* alloc (Packet* first_pkt, uint32_t pkt_cnt, 
                uint16_t status_mask);
  Chunk* alloc (Packet* first_pkt, uint32_t pkt_cnt, 
                const ChunkStatus &status_mask);

  Chunk* alloc (Vector<Packet*> *pkt_vec, uint16_t status_mask);
  Chunk* alloc (Vector<Packet*> *pkt_vec, const ChunkStatus &status_mask);
  
  Chunk* alloc_dup (Chunk *chunk); 
  
  bool addToDstGUIDTable(uint32_t cid);
  bool removeFromDstGUIDTable(uint32_t cid, uint32_t dst_GUID);

  void push_to_port(Packet *p, uint32_t port, uint32_t delay_msec = 0); 

  void setChunkStat(uint32_t chunk_id, uint16_t status_mask); 
  void setChunkStat(uint32_t chunk_id, const ChunkStatus &status_mask);

  Chunk* get(uint32_t chunkId, uint16_t status_mask); 
  Chunk* get(uint32_t chunkId, const ChunkStatus &status_mask );
 
  uint32_t get(uint32_t dst_GUID, uint16_t status_mask, Vector<Chunk*>&); 
  uint32_t get(uint32_t dst_GUID, const ChunkStatus &status_mask, Vector<Chunk*>&);

  //uint32_t remove(uint32_t chunkId);
  
  int removeData(Chunk *chunk);  
  
  //periodically clean up chunkMnager
  void cleanup(); 

 private:
  typedef struct {
    MF_ChunkManager *me;
    Packet * pkt;
    uint32_t port_num;
  } timer_data_t;

  static void handleDelayPush(Timer *, void *data);
  typedef HashTable< uint32_t, Vector<Chunk *>* > DstGUIDTable; 
  DstGUIDTable *_dstGUIDTable;
  typedef HashTable< uint32_t, Chunk *> ChunkIDTable;
  ChunkIDTable *_chunkIDTable;
                
  uint32_t chunk_id;
         
  pthread_mutex_t _chunkIDTable_mtx;
  pthread_mutex_t _dstGUIDTable_mtx; 
  
  Task _cleanup_task;
  Timer _cleanup_timer;
  
  bool _cleanup_running_flag; 
  uint32_t _cleanup_chk_cnt;
  uint32_t _cleanup_chk_id; 
  
  //configurable parameters 
  uint32_t _cleanup_threshhold;
  uint32_t _cleanup_percentage; 
  uint32_t _cleanup_period_sec; 
  
  MF_Logger logger;
};

CLICK_ENDDECLS

#endif /* MF_CACHEMANAGER_HH_ */
