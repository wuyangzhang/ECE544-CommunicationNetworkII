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

#include "mfbitratecache.hh"

CLICK_DECLS

MF_BitrateCache::MF_BitrateCache() 
    : isAccessPoint(false), logger(MF_Logger::init()) {
  bitrateTable = new BitrateTable();
  //if is Not AP
  localBitrateTable = NULL; 
}

MF_BitrateCache::~MF_BitrateCache() {
}

int MF_BitrateCache::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "ACCESS_POINT", cpkP, cpBool, &isAccessPoint, 
                   cpEnd) < 0) {
    return -1; 
  }
  return 0; 
}

int MF_BitrateCache::initialize(ErrorHandler *errh) {
  if (isAccessPoint) {
    logger.log(MF_INFO, 
               "bitrate_cache: Access Router, LocalBitrateTable is enabled");
    localBitrateTable = new LocalBitrateTable(); 
  }
}

uint32_t MF_BitrateCache::size() {
  return bitrateTable->size(); 
}

bool MF_BitrateCache::insert (uint32_t client_guid, uint32_t router_guid, 
                              double mbps, uint32_t valid_time_sec) {
  if (mbps == 0 || client_guid == 0 || router_guid == 0) {
    logger.log(MF_DEBUG, 
      "bitrate_tbl: bitrate = 0, or client_guid = 0, or edge_router_guid = 0"); 
    return false;           
  }

  client_router_guid_t guid_pair;
  guid_pair = make_pair(client_guid, router_guid); 

  client_bitrate_t bitrate;
  bitrate.bitrate = mbps;
  timer_data_t *timerdata = new timer_data_t();
  timerdata->me = this;
  timerdata->guid_pair = guid_pair; 
  bitrate.expiry = new Timer(&MF_BitrateCache::handleExpiry, timerdata);
  bitrate.expiry->initialize(this);
  if (valid_time_sec != 0) {
    bitrate.expiry->schedule_after_sec(valid_time_sec); 
  } else {
    logger.log(MF_DEBUG,
      "bitrate_tbl: valid_time is set to DEFAULT_BITRATE_TIMEOUT"); 
    bitrate.expiry->schedule_after_sec(DEFAULT_BITRATE_TIMEOUT); 
  } 
  
  bitrateTable->set(guid_pair, bitrate);
  logger.log(MF_DEBUG,
    "bitrate_tbl: insert record, client guid: %u, edge router guid: %u, "
    "bitrate: %u, valid sec: %u",
    client_guid, router_guid, mbps, valid_time_sec); 
  return true; 
}

bool MF_BitrateCache::insert(const client_router_guid_t &guid_pair, 
                             const client_bitrate_t &bitrate) {
  //double mbps = bitrate.bitrate;
  //
  //if (mbps == 0) {
  //  return false; 
  //}
  //if (bitrate.expiry == NULL) {
  //  
  //}
  return true; 
}

double MF_BitrateCache::getBitrate(uint32_t client_guid, 
                                   uint32_t edge_router_guid) {
  client_router_guid_t guid_pair = make_pair(client_guid, edge_router_guid); 
  return getBitrate(guid_pair); 
}

double MF_BitrateCache::getBitrate (const client_router_guid_t &guid_pair) {
  BitrateTable::iterator it = bitrateTable->find(guid_pair);
  if (it != bitrateTable->end()) {    //entry exists
    double mbps = it.value().bitrate; 
    logger.log(MF_DEBUG, 
      "bitrate_tbl: bitrate of client guid: %u at router guid: %u is %f",
      guid_pair.first, guid_pair.second, mbps);
    return mbps; 
  } else {
    logger.log(MF_DEBUG,
      "bitrate_tbl: cannot get bitrate of client guid %u at router guid: %u",
      guid_pair.first, guid_pair.second); 
    return 0;
  }
}

void MF_BitrateCache::handleExpiry(Timer *timer, void *data) {
  timer_data_t *timerdata = (timer_data_t*)data;
  assert(timerdata);
  timerdata->me->expire(timerdata->guid_pair, timerdata); 
}

void MF_BitrateCache::expire(const client_router_guid_t &guid_pair, 
                             timer_data_t *timerdata) {
  BitrateTable::iterator it = bitrateTable->find(guid_pair);
  if (it != bitrateTable->end()) {   //entry exists
    delete it.value().expiry;
    bitrateTable->erase(it);
    logger.log(MF_DEBUG, 
      "bitrate_tbl: Timer expires, DELETING guid_pair client guid: %u "
      "edge_router_guid: %u", 
      guid_pair.first, guid_pair.second); 
  } else {
    logger.log(MF_ERROR,
      "bitrate_tbl: Timer expires, but entry guid_pair client guid: %u "
      "edge_router_guid: %u doesn't exist",
      guid_pair.first, guid_pair.second); 
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_BitrateCache)
ELEMENT_REQUIRES(userlevel)
