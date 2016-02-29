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
 * MF_ToFile.cc
 * MF_Aggregator -> MF_ToFile. MF_ToFile write payload of data packets
 * into a binary file, and it takes as input a routing layer datagram. 
 *  Created on: Jun 17, 2011
 *      Author: Kai Su
 *     Aggregator -> ToFile;
 *********************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mftofile.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_ToFile::MF_ToFile()
    : _task(this), recvd_chunks(0), recvd_pkts(0), recvd_bytes(0), 
      logger(MF_Logger::init()) {
  _writingQ = new Vector<Chunk*>();
  pthread_mutex_init(&_writingQ_lock, NULL); 
}

MF_ToFile::~MF_ToFile() {
}

int MF_ToFile::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   /* dir path where recvd files are to be stored */
                   /* TODO make this arg optional */
                   "FOLDER", cpkP+cpkM, cpString, &save_dirpath,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   cpEnd)<0) {
    return -1;
  }

  /* autogenerate unique file name if not known otherwise */
  char filename[20];
  gen_random_filename(filename, 10);

  if (save_dirpath.equals("", -1)) {
    /* no folder specified, use default */
    struct stat fs;
    if (stat(INCOMING_FILES_FOLDER, &fs) || !S_ISDIR(fs.st_mode)) {
      logger.log(MF_FATAL, 
        "to_file: Default file folder '%s' invalid!", 
        INCOMING_FILES_FOLDER);
      exit(-1);
    }
 
    sprintf(filepath, "%s/%s", INCOMING_FILES_FOLDER, filename);
  } else {
    // check if valid folder path
    struct stat fs;
    if (stat(save_dirpath.c_str(), &fs) || !S_ISDIR(fs.st_mode)) {
      logger.log(MF_FATAL, 
        "to_file: Specified folder '%s' invalid!", 
        save_dirpath.c_str());
      exit(-1);
    }
   
    /* use specified folder for saving file */
    sprintf(filepath, "%s/%s", save_dirpath.c_str(), filename);
  }
  return 0;
}

int MF_ToFile::initialize(ErrorHandler *errh) {
  ScheduleInfo::initialize_task(this, &_task, true, errh);
}

bool MF_ToFile::run_task(Task *) {
  Chunk *chunkToWrite = NULL;

  pthread_mutex_lock(&_writingQ_lock); 
  Vector<Chunk*>::iterator it = _writingQ->begin(); 
  while (it != _writingQ->end()) {
    chunkToWrite = *it; 
    if (chunkToWrite->getStatus().isComplete()) {
// && 
//            chunkToWrite->getStatus().isAcked()) {
      logger.log(MF_TIME,
        "to_file: RECEIVED full chunk, chunk_id: %u", 
        chunkToWrite->getChunkID()); 
      if (writeChunkToFile(chunkToWrite) < 0) {
        break; 
      }
      it = _writingQ->erase(it);
      continue; 
    }
    ++it; 
  }
  pthread_mutex_unlock(&_writingQ_lock); 
  _task.reschedule();
  return true; 
}

void MF_ToFile::gen_random_filename(char* s, const int len) {

  static const char alphanum[] ="0123456789"
		                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		                "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  s[len] = 0;
}

void MF_ToFile::push(int port, Packet *p){

  if (port != 0) {
    logger.log(MF_FATAL, "to_file: recvd pkt on unsupported port!");
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
  logger.log(MF_DEBUG, 
    "to_file: Got chunk with chunk_id %u from ChunkManger, "
    "Push it to writingQ", 
    chunk_id);
  pthread_mutex_lock(&_writingQ_lock); 
  _writingQ->push_back(chunk); 
  pthread_mutex_unlock(&_writingQ_lock); 
  p->kill(); 
}

int MF_ToFile::writeChunkToFile(Chunk *chunk) {
  
  FILE *fp;
  long size = 0;
  fp = fopen(filepath, "rb");
  
  /* check current size of file if exists */
  if (fp != NULL) {
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
  } else {
    logger.log(MF_WARN,
      "to_file: file '%s' doesn't exist, will be created", filepath);
  }
 
  Vector<Packet*> *cur_vec = chunk->allPkts(); 

  routing_header *rheader = chunk->routingHeader();
  uint32_t l4_pld_pos = rheader->getPayloadOffset();

  logger.log(MF_DEBUG, "L4 Header Pos: %u\n", l4_pld_pos); 
  transport_header *theader = (transport_header*)((*cur_vec)[0]->data()
                                   + HOP_DATA_PKT_SIZE + l4_pld_pos);


  uint32_t src_guid = rheader->getSrcGUID().to_int(); 
  uint32_t tp_chunk_ID = ntohl(theader->chunkID); 
  logger.log(MF_DEBUG, 
    "to_file: write chunk from guid: '%u' chunk_id: %u transport chunk#: %u", 
    src_guid, chunk->getChunkID(), tp_chunk_ID);

  /* 
   * During recurring transfers, file is appended. Truncate
   * after threshold to prevent disk overrun 
   */
  if (size < 209715200) { 
		//size less than 200MB
		//open in append mode
    logger.log(MF_DEBUG, 
      "to_file: Opening file '%s' in append mode", filepath);
    fp = fopen(filepath, "ab");
  } else {
		//else truncate and start from beginning
    logger.log(MF_INFO, 
      "to_file: File '%s' exists, but too large.Truncating and appending.",  
      filepath);
    fp = fopen(filepath, "wb");
  }
 
  if (fp == NULL) {
    logger.log(MF_ERROR, 
      "to_file: unable to open file '%s' for write", filepath);
      //_chunkManager->remove(chunk->getChunkID());
      _chunkManager->removeData(chunk); 
    return -1;	
  }
  uint32_t res = 0;
  uint32_t file_pld_size = 0; 
  hop_data_t *cur_pkt = NULL; 
  char *payload = NULL;
  Vector<Packet*>::iterator it_end = cur_vec->end(); 
  for (Vector<Packet*>::iterator it = cur_vec->begin(); it != it_end; ++it) {
    cur_pkt = (hop_data_t*)((*it)->data());
    if (ntohl(cur_pkt->seq_num) == 0) {

      payload = (char*)(theader+1);
      file_pld_size = ntohl(cur_pkt->pld_size) - 
                      (l4_pld_pos + TRANS_HEADER_SIZE); 
      logger.log(MF_DEBUG,
        "tofile: first packet data to write %u", file_pld_size); 
    } else {
      payload = cur_pkt->data; 
      file_pld_size = ntohl(cur_pkt->pld_size);
    }
    res = fwrite(payload, 1, file_pld_size, fp);
    if (res != file_pld_size) {
      logger.log(MF_ERROR, "to_file: write was incomplete"); 
    }
    written_bytes += res;
    recvd_bytes += file_pld_size;
    ++recvd_pkts;  
  }
  
  fclose(fp);
  
  ++recvd_chunks;
  logger.log(MF_TIME, 
    "to_file: SAVED chunk; Total recvd chunks/pkts/bytes: %u/%u/%llu "
    "written: bytes: %llu",
    recvd_chunks, recvd_pkts, recvd_bytes, written_bytes);
  //if (_chunkManager->remove(chunk->getChunkID())) {
  if (_chunkManager->removeData(chunk)) {
    return 0; 
  } else {
    return -1; 
  }
}

CLICK_DECLS
EXPORT_ELEMENT(MF_ToFile)
ELEMENT_REQUIRES(userlevel)
