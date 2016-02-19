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

#include "gnrsrespcache.hh"

CLICK_DECLS
GNRS_RespCache:: GNRS_RespCache() : logger(MF_Logger::init()) {
  _respCache = new RespCache();
  pthread_mutex_init(&cacheLock, NULL); 
}

GNRS_RespCache:: ~GNRS_RespCache() {
}

int GNRS_RespCache::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh, 
        cpEnd) < 0) {
    return -1; 
  }
  return 0; 
}

bool GNRS_RespCache::insert(GUID_t guid, Packet *resp_pkt, uint32_t valid_sec) {
  char buf[GUID_LOG_BUF_SIZE]; 
  gnrs_resp_cache_record_t *record = 
      (gnrs_resp_cache_record_t*)malloc(sizeof(gnrs_resp_cache_record_t));  
  record->resp_pkt = resp_pkt;
  timer_data_t *timerdata = new timer_data_t();
  timerdata->me = this;
  timerdata->guid.init(guid);
  record->expiry = new Timer(&GNRS_RespCache::handleExpiry, timerdata);
  record->expiry->initialize(this);
  record->expiry->schedule_after_sec(valid_sec); 
  //record->ts = Timestamp::now_steady();
  String guid_str(guid.getGUID(), GUID_LENGTH);  

  pthread_mutex_lock(&cacheLock); 
  RespCache::iterator it = _respCache->find(guid_str);
  if (it != _respCache->end()) {
    delete it.value()->expiry;
    it.value()->resp_pkt->kill();
    delete it.value();
    _respCache->set(guid_str, record); 
    pthread_mutex_unlock(&cacheLock);
    logger.log(MF_DEBUG,
      "gnrs_resp_cache: update existing entry guid: %s",
      guid.to_log(buf)); 
    return 0; 
  } else {
    _respCache->set(guid_str, record);
    pthread_mutex_unlock(&cacheLock); 
    logger.log(MF_DEBUG, 
      "gnrs_resp_cache: insert new gnrs_resp to entry guid: %s", 
      guid.to_log(buf));
    return 1;            //add new entry;
  }
}

bool GNRS_RespCache::get_response(GUID_t guid, gnrs_lkup_resp_t *resp) {
  assert(resp);
  
  String guid_str(guid.getGUID(), GUID_LENGTH); 
  
  pthread_mutex_lock(&cacheLock);
  RespCache::iterator it = _respCache->find(guid_str);

  if (it == _respCache->end()) {
    logger.log(MF_DEBUG,
      "gnrs_resp_cache: no guid %s entry in respCache", guid_str.c_str()); 
    pthread_mutex_unlock(&cacheLock); 
    //return NULL; 
    return false; 
  } else {
    if (!it.value()->resp_pkt) {
      logger.log(MF_ERROR, 
        "gnrs_resp_cache: resp_pkt is NULL");
      pthread_mutex_unlock(&cacheLock);
      //return NULL; 
      return false; 
    }
    //gnrs_lkup_resp_t *resp = (gnrs_lkup_resp_t*)(it.value()->resp_pkt->data());
    memcpy(resp, (void*)it.value()->resp_pkt->data(), sizeof(gnrs_lkup_resp_t));
    pthread_mutex_unlock(&cacheLock); 
    char buf[GUID_LOG_BUF_SIZE];
    logger.log(MF_DEBUG, 
      "gnrs_resp_cache: find gnrs_lkup_resp of guid %s", guid.to_log(buf));
    return true; 
  } 
}

uint32_t GNRS_RespCache::erase(GUID_t guid) {
  String guid_str(guid.getGUID(), GUID_LENGTH);
  char buf[GUID_LOG_BUF_SIZE]; 
  pthread_mutex_lock(&cacheLock);
  RespCache::iterator it = _respCache->find(guid_str);
  if (it == _respCache->end()) {
    pthread_mutex_unlock(&cacheLock);
    logger.log(MF_DEBUG, 
      "gnrs_resp_cache: no guid: %u entry", guid.to_log(buf));
    return 0; 
  } else {
    delete it.value()->expiry;
    it.value()->resp_pkt->kill();
    delete it.value();
    _respCache->erase(it); 
    pthread_mutex_unlock(&cacheLock);
    logger.log(MF_DEBUG,
      "gnrs_resp_cache: delete guid: %s entry", guid.to_log(buf));
    return 1; 
  }
}

void GNRS_RespCache::handleExpiry(Timer *timer, void *data) {
  timer_data_t *timerdata = (timer_data_t*)data; 
  assert(timerdata);
  timerdata->me->expire(timerdata->guid, timerdata); 
}

void GNRS_RespCache::expire(GUID_t guid, timer_data_t *timerdata) {
  //ease entry from respCache if expired
  char buf[GUID_LOG_BUF_SIZE]; 
  logger.log(MF_DEBUG, 
    "gnrs_resp_cache: gnrs_resp of guid: %s expires, delete entry",
    guid.to_log(buf)); 
  erase(guid);
  delete timerdata; 
}

CLICK_ENDDECLS
EXPORT_ELEMENT(GNRS_RespCache)
ELEMENT_REQUIRES(userlevel)
