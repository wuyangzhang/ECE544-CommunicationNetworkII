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
#ifndef GNRS_RESP_CACHE_HH_
#define GNRS_RESP_CACHE_HH_

#include <click/element.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>
#include <click/timer.hh>

#include <pthread.h>

#include "mf.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

#define GNRS_RESP_TIMEOUT_SEC 10

class GNRS_RespCache : public Element {
 public:
  GNRS_RespCache();
  ~GNRS_RespCache();
  const char *class_name() const           { return "GNRS_RespCache"; }
  const char *port_count() const           { return "0/0"; }
  const char *processing() const           { return "h/h"; }   
  
  int configure(Vector<String>&, ErrorHandler *); 
  
  bool insert(GUID_t guid, Packet *resp_pkt, 
              uint32_t valid_sec = GNRS_RESP_TIMEOUT_SEC);
  bool get_response(GUID_t guid, gnrs_lkup_resp_t *resp); 
  uint32_t erase(GUID_t guid); 
   
 private:
  static void handleExpiry(Timer *, void *); 
  typedef struct TimerData {
    GNRS_RespCache *me;
    GUID_t guid; 
  } timer_data_t;
  void expire(GUID_t guid, timer_data_t *timerdata); 
  pthread_mutex_t cacheLock; 
  
  typedef struct {
    Packet *resp_pkt;
    Timer *expiry;
  } gnrs_resp_cache_record_t;
   
  typedef HashTable<String, gnrs_resp_cache_record_t*> RespCache; 
  RespCache *_respCache; 
  MF_Logger logger; 
}; 

CLICK_ENDDECLS

#endif /*GNRS_RESP_CACHE_HH_*/
