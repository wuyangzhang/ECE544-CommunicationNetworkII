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
/*********************************************************************
 * MF_ChunkSink
 * Receives, logs and destroys chunks.
 *********************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/standard/scheduleinfo.hh>
#include <click/vector.hh>

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mfvirtualchunksink.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_VirtualChunkSink::MF_VirtualChunkSink()
    : _task(this), recvd_chunks(0), recvd_pkts(0), recvd_bytes(0), 
      start_msg_recvd(false), logger(MF_Logger::init()) {
  _chunkQ = new Vector<Chunk*>();
  pthread_mutex_init(&_chunkQ_lock, NULL); 
}

MF_VirtualChunkSink::~MF_VirtualChunkSink() {

}

int MF_VirtualChunkSink::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   cpEnd)<0) {
    return -1;
  }
  return 0;
}

int MF_VirtualChunkSink::initialize(ErrorHandler *errh) {
  ScheduleInfo::initialize_task(this, &_task, true, errh); 
}

bool MF_VirtualChunkSink::run_task(Task *) {
  Chunk *chunk = NULL;
  pthread_mutex_lock(&_chunkQ_lock);
  Vector<Chunk*>::iterator it = _chunkQ->begin();
  while (it != _chunkQ->end()) {
    chunk = *it;
    if (chunk->getStatus().isComplete()) {
      //logger.log(MF_TIME,
      //  "chnk_snk: RECEIVED full Chunk, chunk_id: %u",
      //  chunk->getChunkID());
      sink(chunk); 
      it = _chunkQ->erase(it);
      continue; 
    }
    ++it; 
  }
  pthread_mutex_unlock(&_chunkQ_lock);
  _task.reschedule();
  return true; 
}


void MF_VirtualChunkSink::push(int port, Packet *p){

  if (port != 0) {
    logger.log(MF_FATAL, "vchnk_snk: recvd pkt on unsupported port!");
    exit(-1);
  }

  chk_internal_trans_t* chk_trans = (chk_internal_trans_t*)p->data();
  uint32_t sid = chk_trans->sid;
  uint32_t chunk_id = chk_trans->chunk_id;
  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
  if (!chunk) {
    logger.log(MF_ERROR, "to_file: Got a invalid chunk from ChunkManager");
    return;
  }
  //if no start measurement msg received
    routing_header *rheader = chunk->routingHeader();
    uint32_t vsrc_guid = rheader->getSrcGUID().to_int(); 
    if (rheader->getUpperProtocol() == VIRTUAL_DATA) {
      logger.log(MF_TIME, "vchnk_snk: received chunk from virtual src_guid: %u", vsrc_guid); 
      start_msg_recvd = true;
    } 

  pthread_mutex_lock(&_chunkQ_lock); 
  _chunkQ->push_back(chunk); 
  pthread_mutex_unlock(&_chunkQ_lock); 
  p->kill(); 
}

int MF_VirtualChunkSink::sink(Chunk *chunk) {
  Vector<Packet*> *cur_vec = chunk->allPkts(); 

  routing_header *rheader = chunk->routingHeader(); 
  if (rheader->getUpperProtocol() != VIRTUAL_DATA) {
    //return _chunkManager->remove(chunk->getChunkID());
    return _chunkManager->removeData(chunk); 
  }

  uint32_t src_guid = rheader->getSrcGUID().to_int();
  logger.log(MF_TIME, 
    "chnk_snk: RECEIVED chunk from guid: '%u' chunk_id: %u", src_guid, chunk->getChunkID());
  //if (_chunkManager->remove(chunk->getChunkID())) {
  if (_chunkManager->removeData(chunk)) {  
    return 0;
  } else {
    return -1;
  }
}

CLICK_DECLS
EXPORT_ELEMENT(MF_VirtualChunkSink)
ELEMENT_REQUIRES(userlevel)
