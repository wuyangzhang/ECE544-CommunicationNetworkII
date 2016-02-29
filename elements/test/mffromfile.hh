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
 * MF_FromFile.hh
 *
 *  Created on: Jun 17, 2011
 *      Author: Kai Su
 */

#ifndef MF_FROMFILE_HH_
#define MF_FROMFILE_HH_

#include <click/element.hh>
#include <click/task.hh>
#include <click/timer.hh>
#include <click/notifier.hh>
#include "mf.hh"
#include "mfchunkmanager.hh"
#include "mflogger.hh"

/* default eth packet size if unspecified */
#define PKT_SIZE 1400

CLICK_DECLS

class MF_FromFile : public Element {
 public:
  MF_FromFile();
  ~MF_FromFile();

  const char *class_name() const		{ return "MF_FromFile"; }
  const char *port_count() const		{ return "0/1"; }
  const char *processing() const		{ return "h/h"; }

  int configure(Vector<String>&, ErrorHandler *);
  int initialize(ErrorHandler *);
  void read(char *filename);

  void add_handlers();
  bool run_task(Task *);

 private:
  Task _task;
  Timer _timer;
  bool _active;
  /* to check if downstream has capacity */
  NotifierSignal _signal;

  uint32_t _dst_GUID;
  uint32_t _src_GUID;
  uint32_t _chk_size;
  uint32_t _chk_intval_msec;
  uint32_t _delay_sec;
  uint32_t _pkt_size;
  uint32_t _sid; 
       
  GUID *_my_guid;
  String _my_guid_file; 
  /* monotonically increasing unique chunk identifier, starts at 1 */
  
  uint32_t _curr_chk_ID;
  
  String _file_to_send;
  FILE *_file;
  /* 
   * File offset to resume once the sending task is
   * resumed following a reschedule
   */
  uint32_t curr_file_offset;
  uint32_t file_size; 
   
  MF_ChunkManager *_chk_manager; 
  MF_Logger logger;
};

CLICK_ENDDECLS
#endif /* MF_FILE_HH_ */
