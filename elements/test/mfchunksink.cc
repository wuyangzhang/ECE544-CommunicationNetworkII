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

#include "mfchunksink.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_ChunkSink::MF_ChunkSink()
    : _task(this), recvd_chunks(0), recvd_pkts(0), recvd_bytes(0), 
      start_msg_recvd(false), logger(MF_Logger::init()) {
  _chunkQ = new Vector<Chunk*>();
  pthread_mutex_init(&_chunkQ_lock, NULL); 
}

MF_ChunkSink::~MF_ChunkSink() {

}

int MF_ChunkSink::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   cpEnd)<0) {
    return -1;
  }
  return 0;
}

int MF_ChunkSink::initialize(ErrorHandler *errh) {
  ScheduleInfo::initialize_task(this, &_task, true, errh); 
}

bool MF_ChunkSink::run_task(Task *) {
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


void MF_ChunkSink::push(int port, Packet *p){

  if (port != 0) {
    logger.log(MF_FATAL, "chnk_snk: recvd pkt on unsupported port!");
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
  if (!start_msg_recvd) {
    routing_header *rheader = chunk->routingHeader();
    if (rheader->getUpperProtocol() == MEASUREMENT) {
      start_measurement_msg_t *start_pkt = 
         (start_measurement_msg_t*)(chunk->allPkts()->at(0)->data() + 
         HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
      num_to_measure = ntohl(start_pkt->num_to_measure);
      start = Timestamp::now_steady();
      logger.log(MF_TIME,
        "chnk_snk: start to measuring throughput of %u chunks", num_to_measure); 
      start_msg_recvd = true;
    } 
  }
  pthread_mutex_lock(&_chunkQ_lock); 
  _chunkQ->push_back(chunk); 
  pthread_mutex_unlock(&_chunkQ_lock); 
  p->kill(); 
}

int MF_ChunkSink::sink(Chunk *chunk) {
  Vector<Packet*> *cur_vec = chunk->allPkts(); 

  routing_header *rheader = chunk->routingHeader(); 
  if (rheader->getUpperProtocol() != TRANSPORT) {
    //return _chunkManager->remove(chunk->getChunkID());
    return _chunkManager->removeData(chunk); 
  }
  transport_header *theader = (transport_header*)((*cur_vec)[0]->data() +
                                     ROUTING_HEADER_SIZE );
  char *payload = (char*)(theader+1);

  uint32_t src_guid = rheader->getSrcGUID().to_int();
  uint32_t tp_chunk_ID = ntohl(theader->chunkID); 
  logger.log(MF_TIME, 
    "chnk_snk: RECEIVED chunk from guid: '%u' chunk_id: %u transport chunk# %u",
    src_guid, chunk->getChunkID(), tp_chunk_ID);

  uint32_t pld_size = 0; 
  hop_data_t *cur_pkt = NULL;
  Vector<Packet*>::iterator it_end = cur_vec->end(); 
  for (Vector<Packet*>::iterator it = cur_vec->begin(); it != it_end; ++it) {
    cur_pkt = (hop_data_t*)((*it)->data());
    if (ntohl(cur_pkt->seq_num) == 0) {
      pld_size = ntohl(cur_pkt->pld_size) -
                      (ROUTING_HEADER_SIZE + TRANS_HEADER_SIZE);
    } else {
      pld_size = ntohl(cur_pkt->pld_size); 
    } 
    recvd_bytes += pld_size;
    ++recvd_pkts; 
  }

  ++recvd_chunks;
  logger.log(MF_TIME, "chnk_snk: CONSUMED chunk; "
    "Total recvd chunks/pkts/bytes: %u/%u/%llu ",
    recvd_chunks, recvd_pkts, recvd_bytes);
  if (recvd_chunks == num_to_measure && start_msg_recvd) {
    end = Timestamp::now_steady(); 
    computeThroughput(); 
  }
  //if (_chunkManager->remove(chunk->getChunkID())) {
  if (_chunkManager->removeData(chunk)) {  
    return 0;
  } else {
    return -1;
  }
}

double MF_ChunkSink::computeThroughput() {
  logger.log(MF_INFO, "end: %u, %u, start: %u, %u", 
    end.sec(), end.msec(), start.sec(), start.msec());
  double t_msec = 0;
  double t_sec = 0; 
  if (end.msec() > start.msec()) {
    t_msec = end.msec() - start.msec();
    t_sec = (end.sec() - start.sec()) + t_msec / 1000.0; 
  } else {
    t_msec = start.msec() - end.msec(); 
    t_sec = (end.sec() - start.sec()) - t_msec / 1000.0;
  }
  logger.log(MF_INFO,
    "chnk_snk: Receiving %u chunks takes %2.4f sec", num_to_measure, t_sec);
  double throughput_mbps = (recvd_bytes * 8 / t_sec) / (1024 * 1024);
  logger.log(MF_INFO,
    "chuk_snk: THROUGHPUT: %2.4f mbps", throughput_mbps);
  return throughput_mbps; 
}



CLICK_DECLS
EXPORT_ELEMENT(MF_ChunkSink)
ELEMENT_REQUIRES(userlevel)
