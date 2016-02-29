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
#ifndef MF_BITRATECACHE_HH_
#define MF_BITRATECACHE_HH_

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/timer.hh>

#include "mflogger.hh"

#define DEFAULT_BITRATE_TIMEOUT 30
CLICK_DECLS

typedef struct {
  double bitrate;
  Timer *expiry;
} client_bitrate_t; 

typedef Pair<uint32_t, uint32_t> client_router_guid_t; 

class MF_BitrateCache : public Element {
 public:
  MF_BitrateCache();
  ~MF_BitrateCache();
  const char* class_name() const               { return "MF_BitrateCache"; }
  const char* port_count() const               { return "0/0"; }
  const char* processing() const               { return AGNOSTIC; }
  
  int configure(Vector<String> &conf, ErrorHandler *errh); 
  int initialize(ErrorHandler *); 
  
  uint32_t size(); 
  bool insert (uint32_t client_guid, uint32_t edge_router_guid, double bitrate, 
               uint32_t valid_time_sec = DEFAULT_BITRATE_TIMEOUT);
  bool insert (const client_router_guid_t &guid_pair, 
               const client_bitrate_t &bitrate);
  
  double getBitrate (uint32_t client_guid, uint32_t edge_router_guid);
  double getBitrate (const client_router_guid_t &guid_pair); 
   
 private:
  static void handleExpiry(Timer*, void *); 
  typedef struct TiemrData {
    MF_BitrateCache *me;
    client_router_guid_t guid_pair; 
  } timer_data_t;
  void expire(const client_router_guid_t &guid_pair, timer_data_t *timerdata); 
  typedef HashTable<client_router_guid_t, client_bitrate_t> BitrateTable; 
  BitrateTable *bitrateTable;
  
  bool isAccessPoint; 
  typedef HashTable<uint32_t, uint32_t> LocalBitrateTable;
  LocalBitrateTable *localBitrateTable; 
  MF_Logger logger; 
}; 

CLICK_ENDDECLS
#endif  /*MF_BITRATECACHE_HH_*/
