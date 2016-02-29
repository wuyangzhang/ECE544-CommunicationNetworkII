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
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>

#include "gnrsreqresphandler.hh"
#include "gnrsmessages.hh"
#include "gnrsnetaddr.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

GNRS_ReqRespHandler::GNRS_ReqRespHandler() 
    : _req_id(0), logger(MF_Logger::init()) {
  _reqCache = new ReqCache();
  pthread_mutex_init(&reqCacheLock, NULL);
}

GNRS_ReqRespHandler::~GNRS_ReqRespHandler() {
}

/** 
 * Required params: 
 * 1.) MY_GUID: self GUID
 * 2.) NET_ID: network id 
 * 3.) RESP_LISTEN_IP: IP tos listen on for GNRS server responses 
 * 4.) RESP_LISTEN_PORT: port to listen on for GNRS server responses 
 */
int GNRS_ReqRespHandler::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpUnsigned, &my_guid,
        "NET_ID", cpkP+cpkM, cpString, &net_id,
        "RESP_LISTEN_IP", cpkP+cpkM, cpString, &my_ip,
        "RESP_LISTEN_PORT", cpkP+cpkM, cpUnsigned, &my_udp_port,
        "RESP_CACHE", cpkP+cpkM, cpElement, &_respCache,
        cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void GNRS_ReqRespHandler::push(int port, Packet *p) {
  if (port == 0) {
    handleReqPkt(p);
  } else if (port == 1) {
    handleRespPkt(p); 
  } else {
    logger.log(MF_ERROR, 
      "gnrs_rrh: Unrecognized packet on port %d", port);
    p->kill(); 
  }
}

void GNRS_ReqRespHandler::handleReqPkt(Packet *p, uint32_t resend) {
  //gnrs request from a click element in custom format
  gnrs_req *req = (gnrs_req *)p->data();
  uint32_t req_type = req->type;
  GUID_t guid;
  guid.init(req->GUID);
  char buf[GUID_LOG_BUF_SIZE];
  //TODO define NA 
  uint16_t weight = req->weight;
  uint32_t ttl = DEFAULT_MAPPING_TTL_SECS;

  WritablePacket *out_req_pkt = NULL;
  
  if (req_type == GNRS_LOOKUP) {
    req_t req_msg;
    req_msg.version = PROTOCOL_VERSION;
    req_msg.type = LOOKUP_REQUEST;
    //set len later
    req_msg.id = get_request_id();
    char tmp[32];
    sprintf(tmp, "%s:%u", my_ip.c_str(), my_udp_port);
    string addr_s(tmp);
    NetAddr local_addr(NET_ADDR_TYPE_IPV4_PORT, addr_s);
    req_msg.src_addr = local_addr.get_tlv();

    //lookup request payload
    lookup_t lkup; 
    memcpy(lkup.guid, guid.getGUID(), GUID_LENGTH); 
    req_msg.data_len = sizeof(lookup_t);
    req_msg.data = (void*) &lkup;

    //set up options 
    opt_tlv_t opts[1];
    uint16_t& opts_len = req_msg.opts_len;
    uint16_t& num_opts = req_msg.num_opts;
    opts_len = 0;
    num_opts = 0;
    //redirect option: allow request redirection to other GNRS servers
    unsigned char redirect_val[] = {0x00, 0x01};
    opt_tlv_t* redirect_opt = &opts[0];
    redirect_opt->type = OPTION_REQUEST_REDIRECT;
    redirect_opt->len = 2;
    redirect_opt->value = (unsigned char*)&redirect_val;
    opts_len += 2 + redirect_opt->len;
    num_opts++;

    req_msg.opts = opts;

    uint16_t msg_len = 16 + req_msg.src_addr.len 
                + req_msg.data_len + req_msg.opts_len;
    out_req_pkt = Packet::make(0, 0, msg_len, 0);
    assert(out_req_pkt);
    if ((GnrsMessageHelper::build_request_msg(req_msg, 
                    out_req_pkt->data(), out_req_pkt->length())) < 0) {
      /* error building message */  
      logger.log(MF_ERROR, 
        "gnrs_rrh: error building lookup message of len: %u, using pkt buff "
        "of size: %u for guid: %s req id: %u", 
        msg_len, out_req_pkt->length(), guid.to_log(buf), req_msg.id);
    } else {

      /* store request to match responses and revisit 
                 * timedout requests */
      store_request(_req_id, p, resend); 
      output(0).push(out_req_pkt);
      logger.log(MF_DEBUG, 
        "gnrs_rrh: sent lookup for guid: %s req id: %u", 
        guid.to_log(buf), _req_id);
    }
  } else if (req_type == GNRS_INSERT) {
    logger.log(MF_DEBUG, 
      "gnrs_rrh: recvd insert req for guid: %s", guid.to_log(buf));
    /* check if an insert op has been previously performed for 
       this guid. If yes, skip. This is implemented to avoid
       the unworkable append semantics of the insert operation
       on the Java version of the GNRS server */
    if (get_client(guid) > 0) {
      logger.log(MF_DEBUG, 
        "gnrs_rrh: guid %s seen before, skipping insert", guid.to_log(buf));
      p->kill();
      return;
    }
   
    req_t req_msg;
    req_msg.version = PROTOCOL_VERSION;
    req_msg.type = INSERT_REQUEST;
    //set req_msg.len later
    req_msg.id = get_request_id();

    char tmp[32];
    sprintf(tmp, "%s:%u", my_ip.c_str(), my_udp_port);
    string addr_s(tmp);
    NetAddr local_addr(NET_ADDR_TYPE_IPV4_PORT, addr_s);
    req_msg.src_addr = local_addr.get_tlv();

    //insert request payload
    upsert_t ups; 
    memcpy(ups.guid, guid.getGUID(), GUID_LENGTH); 
    int data_len = GUID_LENGTH + 4; 

    //set the one address entry corresponding to this router
    //TODO maybe the NA ought to arrive in the request, and not
    //determined here
    addr_tlv_t addrs_v[1];
    sprintf(tmp, "%s:%u", net_id.c_str(), my_guid);
    addr_s.assign(tmp);
    NetAddr na(NET_ADDR_TYPE_NA, addr_s);
    addrs_v[0] = na.get_tlv();
    data_len += na.len + 4;

    ups.addrs = &addrs_v[0];
    ups.size = 1;

    req_msg.data = (void*) &ups;
    req_msg.data_len = data_len;

    //set up options 
    opt_tlv_t opts[1];
    uint16_t& opts_len = req_msg.opts_len;
    uint16_t& num_opts = req_msg.num_opts;
    opts_len = 0;
    num_opts = 0;

    //redirect option: allow request redirection to other GNRS servers
    unsigned char redirect_val[] = {0x00, 0x01};
    opt_tlv_t* redirect_opt = &opts[0];
    redirect_opt->type = OPTION_REQUEST_REDIRECT;
    redirect_opt->len = 2;
    redirect_opt->value = (unsigned char*)&redirect_val;
    opts_len += 2 + redirect_opt->len;
    num_opts++;

    req_msg.opts = opts;

    uint16_t msg_len = 16 + req_msg.src_addr.len 
                + req_msg.data_len + req_msg.opts_len;

    out_req_pkt = Packet::make(0, 0, msg_len, 0);
    assert(out_req_pkt);
    if ((GnrsMessageHelper::build_request_msg(req_msg, 
                    out_req_pkt->data(), out_req_pkt->length())) < 0) {
      /* error building message */  
      logger.log(MF_ERROR, 
        "gnrs_rrh: error building insert message of len: %u, "
         "using pkt buff of size: %u or guid: %s req id: %u", 
         msg_len, out_req_pkt->length(), guid.to_log(buf), req_msg.id);
    } else {

      /* store request to match responses and revisit 
       * timedout requests */
      store_request(_req_id, p, 0); 
      output(0).push(out_req_pkt);
      logger.log(MF_INFO, 
        "gnrs_rrh: sent insert for guid: %s with req id: %u weight: %u", 
        guid.to_log(buf), _req_id, weight);
      logger.log(MF_INFO, 
        "gnrs_rrh: future inserts for guid %s will be skipped until expiry", guid.to_log(buf));
    }
  } else{//unknown
    logger.log(MF_WARN, 
      "gnrs_rrh: Unknown msg type: %u at in-port 0", req_type);
    p->kill();
    return;
  }
}
void GNRS_ReqRespHandler::handleRespPkt(Packet *p) {
  //udp response packet from GNRS server
  resp_t rsp; 
        
  GnrsMessageHelper::parse_response_msg((unsigned char*)p->data(), p->length(), rsp);

  if (rsp.status == RESPONSE_INCOMPLETE) {
    logger.log(MF_ERROR, 
      "gnrs_rrh: Received incomplete response! len: %u", p->length());
    p->kill();
    return;	
  }
 
  gnrs_req_t * req = get_request(rsp.req_id);
  if (req == NULL) {
    logger.log(MF_ERROR,
      "gnrs_rrh: can't find matching request for resp; req id: %u", rsp.req_id);
    p->kill();
    return;
  }
  GUID_t guid_c;
  guid_c.init(req->GUID);
  GUID_t guid(guid_c);
  char buf[GUID_LOG_BUF_SIZE]; 
	
  if (rsp.type == INSERT_RESPONSE) {
    if (rsp.code == RESPONSE_FAILURE) {
      logger.log(MF_ERROR, 
        "gnrs_rrh: insert failed for req id: %u, response code: %d", 
        rsp.req_id, rsp.code);
      //TODO decide based on response code if should retry 
    } else {
      logger.log(MF_DEBUG, 
        "gnrs_rrh: recvd insert ack for req id: %u, response code: %d", 
         rsp.req_id, rsp.code);
      /* record that client was seen before and the GNRS 
       * was updated with address */
      store_client(guid);
    }
    cleanup_request(rsp.req_id);    
  } else if(rsp.type == LOOKUP_RESPONSE){
    if (rsp.code == RESPONSE_FAILURE) {
      logger.log(MF_ERROR, 
        "gnrs_rrh: Lookup failed for req id: %u, response code: %d", 
        rsp.req_id, rsp.code);
      //TODO decide based on response code if should retry 
      cleanup_request(rsp.req_id); 
      p->kill();
      return;	
    } 
    logger.log(MF_DEBUG, 
      "gnrs_rrh: recvd lookup response req id: %u, response code: %d", 
      rsp.req_id, rsp.code);
    /* prepare internal resp message and send to requestor element */
    WritablePacket *out_resp_pkt = NULL;
    out_resp_pkt = Packet::make(0, 0, sizeof(gnrs_lkup_resp), 0);
    assert(out_resp_pkt);
    memset(out_resp_pkt->data(), 0, out_resp_pkt->length()); 
    gnrs_lkup_resp_t  *resp = (gnrs_lkup_resp_t*)out_resp_pkt->data();
    memcpy(resp->GUID, guid.getGUID(), GUID_LENGTH); 
    int num_na = 0;
    for (int i = 0; (i < rsp.lkup_data.size) && (num_na < LOOKUP_MAX_NA); ++i) {
      //Currently, NA is being encoded as a GUID, so all
      //responses are expected to in TYPE GUID
      if (rsp.lkup_data.addrs[i].type == NET_ADDR_TYPE_GUID) {
        //TODO since we overload NA and GUID types due
        //to GNRS server limitation, the handling will
        //be identical. It works since the GUID currently
        //is a 4 byte encoded as last 4 bytes of 20 bytes,
        //and NA is net(16):local(4), where local is essentially
        //a guid.
        NetAddr na(NET_ADDR_TYPE_NA,
        rsp.lkup_data.addrs[i].value, 
        rsp.lkup_data.addrs[i].len);
        resp->NAs[num_na].net_addr = na.netaddr_local;
        //resp->NAs[num_na].TTL = 
        //resp->NAs[num_na].weight = 
        
        logger.log(MF_DEBUG,
          "gnrs_rrh: lookup resp for guid: %s -> LA: %u", 
          guid.to_log(buf), resp->NAs[num_na].net_addr);
        num_na++;

      } else if (rsp.lkup_data.addrs[i].type == NET_ADDR_TYPE_NA) {

        NetAddr na(NET_ADDR_TYPE_NA, rsp.lkup_data.addrs[i].value, 
                     rsp.lkup_data.addrs[i].len);
        resp->NAs[num_na].net_addr = na.netaddr_local;
        //resp->NAs[num_na].TTL = 
        //resp->NAs[num_na].weight = 
        logger.log(MF_DEBUG,
          "gnrs_rrh: lookup resp for guid: %u -> NA/LA: %s/%u", 
           resp->GUID, na.netaddr_net.c_str(), resp->NAs[num_na].net_addr);
        num_na++;

      } else {
        logger.log(MF_WARN, 
          "gnrs_rrh: Unsupported addr encoding in lookup response; type: %d", 
          rsp.lkup_data.addrs[i].type);
        //skip this entry
        continue;
      }
    } 
    resp->na_num = num_na;
    //push gnrs lookup response to element downstream on port 1
    if (_respCache->insert(guid, out_resp_pkt)) {   //if not a new entry, push pkt to net_binder; 
      output(1).push(out_resp_pkt);
    }
    cleanup_request(rsp.req_id); 
    //TODO cache response if valid
  } else{//unknown 
    logger.log(MF_WARN,"gnrs_rrh: Got unknown msg type at in-port 1");
  }
  p->kill();
}

bool GNRS_ReqRespHandler::store_request(uint32_t req_id, Packet *req_pkt, 
                                        uint32_t resend) {
  gnrs_req_cache_record_t *record =
      (gnrs_req_cache_record_t*)malloc(sizeof(gnrs_req_cache_record_t));
  record->req_pkt = req_pkt;
  timer_data_t *timerdata = new timer_data_t();
  timerdata->me = this;
  timerdata->req_id = req_id;
  timerdata->resend = resend; 
  record->expiry = new Timer(&GNRS_ReqRespHandler::handleReqExpiry, timerdata);
  record->expiry->initialize(this);
  record->expiry->schedule_after_sec(GNRS_REQ_TIMEOUT_SEC);
  logger.log(MF_DEBUG,
    "gnrs_rrh: insert req_id %u", req_id);
  pthread_mutex_lock(&reqCacheLock);
  bool ret = _reqCache->set(req_id, record);
  pthread_mutex_unlock(&reqCacheLock);
  return ret;
}

gnrs_req_t * GNRS_ReqRespHandler::get_request(uint32_t req_id) {
  ReqCache::iterator it = _reqCache->find(req_id);
  if (it == _reqCache->end()) {
    logger.log(MF_DEBUG, "gnrs_rrh: no req_id %u entry", req_id);
    return NULL;
  } else {
    logger.log(MF_DEBUG,
      "gnrs_rrh: find req_id %u record in reqCache", req_id);
    return (gnrs_req_t*)it.value()->req_pkt->data();
  }
}

int GNRS_ReqRespHandler::cleanup_request(uint32_t req_id) {
  ReqCache::iterator it = _reqCache->find(req_id);
  if (it == _reqCache->end()) {
    logger.log(MF_DEBUG, "gnrs_rrh: no req_id %u entry", req_id);
    return 0;
  } else {
    delete it.value()->expiry;
    //kill pakcet
    it.value()->req_pkt->kill(); 
    logger.log(MF_DEBUG, "gnrs_rrh: delete req_id %u", req_id);
    pthread_mutex_lock(&reqCacheLock);
    _reqCache->erase(it);
    pthread_mutex_unlock(&reqCacheLock);
    return 1;
  } 
}

void GNRS_ReqRespHandler::handleReqExpiry(Timer *timer, void *data) {
  timer_data_t *timerdata = (timer_data_t*)data;
  assert(timerdata);
  timerdata->me->reqExpire(timerdata->req_id, timerdata); 
}

void GNRS_ReqRespHandler::reqExpire(uint32_t req_id, timer_data_t *timerdata) {
  ReqCache::iterator it = _reqCache->find(req_id);
  if (it == _reqCache->end()) {
    logger.log(MF_ERROR, 
      "gnrs_rrh: cannot find expired req_id %u entry", req_id); 
    return;
  } else {
    delete it.value()->expiry; 
    if (timerdata->resend <= 0) {
      it.value()->req_pkt->kill();
      logger.log(MF_DEBUG,
        "gnrs_rrh: delete expired req_id %u entry", req_id);
      pthread_mutex_lock(&reqCacheLock);
      _reqCache->erase(it);
      pthread_mutex_unlock(&reqCacheLock);
    } else {
      uint32_t resend = --timerdata->resend; 
      logger.log(MF_DEBUG, 
        "gnrs_rrh: delete expired req_id %u entry, and resend", req_id);
      pthread_mutex_lock(&reqCacheLock);
      _reqCache->erase(it);
      pthread_mutex_unlock(&reqCacheLock);
      //resend a new req_id
      handleReqPkt(it.value()->req_pkt, resend);
    }
  }
}

void GNRS_ReqRespHandler::store_client(GUID_t &client_guid){

  //TODO should add a timestamp for retry management
  String guid_str(client_guid.getGUID(), GUID_LENGTH); 
  HashTable<String, uint32_t>::iterator it = client_map.find(guid_str);
  int prev_cnt = 0;
  if (it != client_map.end()){
    prev_cnt = it.value();
  }
  client_map.set(guid_str, prev_cnt + 1);
}

int GNRS_ReqRespHandler::get_client(GUID &client_guid){
  String guid_str(client_guid.getGUID(), GUID_LENGTH); 
  HashTable<String, uint32_t>::iterator it = client_map.find(guid_str);
  if (it == client_map.end()) {
    return -1; 
  } else {
    return it.value(); 
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(GNRS_ReqRespHandler)
ELEMENT_REQUIRES(userlevel)
