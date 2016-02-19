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
#ifndef MF_BITRATEHANDLER_HH_
#define MF_BITRATEHANDLER_HH_

#include <click/element.hh>
#include <click/etheraddress.hh>

#include "mf.hh"
#include "mfchunkmanager.hh"
#include "mfarptable.hh"
#include "mfbitratecache.hh"
#include "mflogger.hh"

#define DEFAULT_BITRATE_LOOKUP_PORT 6060
#define MAC_BYTE_SIZE 6
//ERROR
//#define CREATE_SOCK_ERROR -1
//#define BIND_SOCK_ERROR -2
//#define SEND_SOCK_ERROR -3

typedef struct {
  uint32_t lookup_id;
  char mac[MAC_BYTE_SIZE]; 
} bitrate_local_lookup_t;

typedef struct {
  uint32_t lookup_id;
  uint32_t bitrate;
  uint32_t valid_sec; 
} bitrate_local_resp_t; 

CLICK_DECLS

class MF_BitrateHandler : public Element {
public:
  MF_BitrateHandler(); 
  ~MF_BitrateHandler();
  
  const char *class_name() const          {return "MF_BitrateHandler";}
  const char *port_count() const          {return "2/2";}
  const char *processing() const          {return "h/h";}
  
  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p); 
private:
  void handleBitrateReqResp(Packet *p); 
  void handleHostBitrate(Packet *p);

  void lookupHostBitrate(EtherAddress ether_addr);
  bool sendBitrateResp (uint32_t dst_GUID, uint32_t dst_NA, uint32_t req_GUID, 
                        double bitrate, uint32_t valid_sec);
  
  uint32_t _myGUID; 
  MF_ChunkManager *_chunkManager;
  MF_BitrateCache *_bitrateCache;
  MF_ARPTable *arp_tbl_p;
    
  uint32_t local_lookup_id;
  //<lookup_id, chunk_id>
  typedef HashTable<uint32_t, Chunk*> LocalLookupTable; 

  LocalLookupTable *lookupTable;  
  
  MF_Logger logger; 
};

CLICK_ENDDECLS
#endif /*MF_BITRATEHANDLER_HH_*/
