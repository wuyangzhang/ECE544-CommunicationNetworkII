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
 * MF_ChunkSink.hh
 */

#ifndef MF_CHUNKSINK_HH_
#define MF_CHUNKSINK_HH_

#include <click/element.hh>
#include <click/task.hh>
#include <click/timestamp.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/string.hh>

#include "mf.hh"
#include "mfchunk.hh"
#include "mfchunkmanager.hh"
#include "mflogger.hh"

CLICK_DECLS

class MF_ChunkSink : public Element {
 public:
  MF_ChunkSink();
  ~MF_ChunkSink();

  const char *class_name() const		{ return "MF_ChunkSink"; }
  const char *port_count() const		{ return "1/0"; }
  const char *processing() const		{ return "h/h"; }

  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p);

  int initialize(ErrorHandler *); 
  bool run_task(Task *); 
  int sink(Chunk *); 
 private:
  double computeThroughput(); 
  //recv stats
  uint32_t recvd_chunks;
  uint32_t recvd_pkts;
  uint64_t recvd_bytes;
 
  bool start_msg_recvd; 
  uint32_t num_to_measure; 
  Timestamp start; 
  Timestamp end;   
   
  Task _task; 
  Vector<Chunk*> *_chunkQ;
  pthread_mutex_t _chunkQ_lock; 
  MF_ChunkManager *_chunkManager; 
  MF_Logger logger;
};

CLICK_ENDDECLS
#endif /* MF_CHUNKSINK_HH_ */
