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
 * MF_RouterStats.hh
 *
 *  Created on: Oct. 4th, 2011
 *      Author: Kai Su
 */

#ifndef MF_ROUTERSTATS_HH_
#define MF_ROUTERSTATS_HH_

#include <click/element.hh>

#include <pthread.h>

#define DEFAULT_WAITING_CHUNK_LIMIT 1000
#define DEFAULT_READY_CHUNK_LIMIT 1000

CLICK_DECLS

class MF_RouterStats : public Element {
 public:
  MF_RouterStats();
  ~MF_RouterStats();

  const char *class_name() const              { return "MF_RouterStats"; }
  const char *port_count() const              { return PORTS_0_0; }
  const char *processing() const              { return AGNOSTIC; }
  int configure(Vector<String> &, ErrorHandler *);
  
  void setNumReadyChunk(uint32_t);
  uint32_t getNumReadyChunk();
  
  void setNumWaitingChunk(uint32_t);
  uint32_t getNumWaitingChunk(); 
  
  bool isBusy(); 

private:
  uint32_t _num_ready_chk;
  uint32_t _num_waiting_chk;
  
  uint32_t _waiting_chk_limit;
  uint32_t _ready_chk_limit; 
  
  pthread_mutex_t _ready_lock;
  pthread_mutex_t _waiting_lock; 
};

CLICK_ENDDECLS

#endif /* MF_ARPTABLE_HH_ */
