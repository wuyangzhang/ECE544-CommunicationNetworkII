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
/********************************************************************
 * MF_RouterStats.cc
 *  Created on: Oct. 4th, 2011
 *      Author: Kai Su
 ********************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "mfrouterstats.hh"

CLICK_DECLS
MF_RouterStats::MF_RouterStats()
    : _num_ready_chk (0), _num_waiting_chk(0), 
      _waiting_chk_limit(DEFAULT_WAITING_CHUNK_LIMIT),
      _ready_chk_limit(DEFAULT_READY_CHUNK_LIMIT) {
  pthread_mutex_init(&_ready_lock, NULL);
  pthread_mutex_init(&_waiting_lock, NULL); 
}

MF_RouterStats::~MF_RouterStats() {
}

int MF_RouterStats::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "WAITING_CHUNK_LIMIT", cpkP, cpUnsigned, &_waiting_chk_limit,
                   "READY_CHUNK_LIMIT", cpkP, cpUnsigned, &_ready_chk_limit,
		   cpEnd) < 0) {
    return -1;
  }
  return 0;
}

bool MF_RouterStats::isBusy() {
  if (_num_waiting_chk >= _waiting_chk_limit || 
        _num_ready_chk >= _ready_chk_limit) {
    return true; 
  } else {
    return false;
  }
}

//TODO: KN: synchronizing the ready count is not necessary unless
// fine grain control is planned
void MF_RouterStats::setNumReadyChunk(uint32_t num_chk) {
  pthread_mutex_lock(&_ready_lock); 
  _num_ready_chk = num_chk;
  pthread_mutex_unlock(&_ready_lock); 
}
 
uint32_t MF_RouterStats::getNumReadyChunk() {
  return _num_ready_chk;
}

void MF_RouterStats::setNumWaitingChunk(uint32_t num_chk) {
  pthread_mutex_lock(&_waiting_lock); 
  _num_waiting_chk = num_chk;
  pthread_mutex_unlock(&_waiting_lock); 
}

uint32_t MF_RouterStats::getNumWaitingChunk() {
  return _num_waiting_chk;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_RouterStats)
ELEMENT_REQUIRES(userlevel)
