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
 * MF_ToFile.hh
 *
 *  Created on: Jun 17, 2011
 *      Author: Kai Su
 */

#ifndef MF_TOFILE_HH_
#define MF_TOFILE_HH_

#include <click/element.hh>
#include <click/task.hh>
#include <click/string.hh>
#include <click/standard/scheduleinfo.hh>


#include "mf.hh"
#include "mflogger.hh"
#include "mfaggregator.hh"

//assumes it exists and is enabled for read/write
#define INCOMING_FILES_FOLDER "/root/file"

CLICK_DECLS

class MF_ToFile : public Element {
 public:
  MF_ToFile();
  ~MF_ToFile();

  const char *class_name() const		{ return "MF_ToFile"; }
  const char *port_count() const		{ return "1/0"; }
  const char *processing() const		{ return "h/h"; }

  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p);
  int initialize(ErrorHandler *); 
  bool run_task(Task *); 
 private:
  Task _task; 
  /* directory for saving received files */
  String save_dirpath;

  /* filepath for storing all chunks recvd in a session */
  char filepath[500];
   
  //recv stats
  uint32_t recvd_chunks;
  uint32_t recvd_pkts;
  uint64_t recvd_bytes;

  uint64_t written_bytes;

  pthread_mutex_t _writingQ_lock; 
  Vector<Chunk*> *_writingQ;
  int writeChunkToFile(Chunk * chunk);
  
  void gen_random_filename(char*, const int);
  MF_ChunkManager *_chunkManager; 
  MF_Logger logger;
};

CLICK_ENDDECLS
#endif /* MF_TOFILE_HH_ */
