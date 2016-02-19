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
#ifndef GNRS_REQRESPHANDLER_HH_
#define GNRS_REQRESPHANDLER_HH_

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/timer.hh>

#include "gnrsrespcache.hh"
#include "mf.hh"
#include "mflogger.hh"

/* duration after which the guid->NA binding should be considered 'expired' */
#define DEFAULT_MAPPING_TTL_SECS 15

#define GNRS_REQ_TIMEOUT_SEC 10
#define GNRS_REQ_DEFAULT_RESEND_TIMES 1

CLICK_DECLS

class GNRS_ReqRespHandler : public Element {
 public:
  GNRS_ReqRespHandler();
  ~GNRS_ReqRespHandler();
  const char *class_name() const		{ return "GNRS_ReqRespHandler"; }
  const char *port_count() const		{ return "2/2"; }
  const char *processing() const		{ return "h/h"; }
    
  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p);

 private:
  /* identifies the network the router belongs to */
  String net_id;

  /* unique router id - used as routing identifier in GUID routing */
  uint32_t my_guid;

  /*TODO: temporary use as IP endpoint ifor GNRS messages; to be
   * removed once GNRS messages are GUID routed */
  String my_ip;

  /* port on which I should listen for responses - required to be noted
   * on request headers */	
  uint32_t my_udp_port;

  /* monotonically increasing id for requests originating from this
   * router. Starts at 1. */
  uint32_t _req_id;

  /* mapping between req id and gnrs request packets received by this element */
  //HashMap<uint32_t, Packet *> req_map;
	
  /* stores statistics about client guid. For now it stores how many time
   * it has seen a particular client GUID
   */
  HashTable<String, uint32_t> client_map;
  //TODO make thread safe?
  uint32_t get_request_id() { return ++_req_id; }
  /* management functions for stored requests */
  int get_client(GUID_t &client_guid);
  void store_client(GUID_t &client_guid);
  
  bool store_request(uint32_t req_id, Packet * req_pkt, uint32_t resend);   
  gnrs_req_t *get_request(uint32_t req_id); 
  int cleanup_request(uint32_t req_id); 
  
  static void handleReqExpiry(Timer *, void *);
  typedef struct TimerData {
    GNRS_ReqRespHandler *me;
    uint32_t req_id;
    uint32_t resend; 
  } timer_data_t;
  void reqExpire(uint32_t req_id, timer_data_t *timerdata); 

  void handleReqPkt(Packet *pkt, 
                    uint32_t resend = GNRS_REQ_DEFAULT_RESEND_TIMES);
  void handleRespPkt(Packet *pkt); 
  
  typedef struct {
    Packet *req_pkt;
    Timer *expiry;
  } gnrs_req_cache_record_t;
  typedef HashTable<uint32_t, gnrs_req_cache_record_t*> ReqCache;
  ReqCache *_reqCache; 
  pthread_mutex_t reqCacheLock; 
  GNRS_RespCache *_respCache;         

  MF_Logger logger;
	
};

CLICK_ENDDECLS
#endif /* GNRS_ReqRespHandler_HH_ */
