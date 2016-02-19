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
#include <click/straccum.hh>
#include <click/args.hh>
#include <click/handlercall.hh>

#include "mfneighbortable.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_NeighborTable::MF_NeighborTable():logger(MF_Logger::init()){
  neighbors = new neighbor_map_t(); 
}

MF_NeighborTable::~MF_NeighborTable() {
}

int MF_NeighborTable::configure(Vector<String> &conf, ErrorHandler *errh) {

  if (cp_va_kparse(conf, this, errh, cpEnd) < 0) {
    return -1;
  }
  return 0;
}


/* 
 * TODO: this is not thread safe. 
 */
bool MF_NeighborTable::insert(uint32_t nbr_guid, 
                              time_t sett_rcvd, uint32_t seqno){

  // if neighbor already exist : update the entry and reset th timer
  neighbor_map_t::iterator it = neighbors->find(nbr_guid);
  if (it != neighbors->end()) {
    it.value()->expiry->unschedule();
    it.value()->s_ett = sett_rcvd;
    it.value()->all_ett[seqno % NEIGHBOR_HISTORY_SIZE] = sett_rcvd; 
    uint32_t prev_seqno = it.value()->prev_seq_no; 
    if (seqno != (prev_seqno + 1)) {
      /* set any missed reporting intervals to defaults */
      uint32_t k = (prev_seqno + 1) % NEIGHBOR_HISTORY_SIZE;
      while (k != (seqno % NEIGHBOR_HISTORY_SIZE)) {
        it.value()->all_ett[k] = 0;
        k = (k + 1) % NEIGHBOR_HISTORY_SIZE;
      }
    }

    /* compute average sett over short term averaging window */
    uint32_t ett_window = 0;
    uint32_t count_window = 0;
    for (int i = seqno % NEIGHBOR_HISTORY_SIZE; i != (seqno + 
           NEIGHBOR_HISTORY_SIZE - SETT_WINDOW_SIZE) % NEIGHBOR_HISTORY_SIZE; 
           i = (i + NEIGHBOR_HISTORY_SIZE - 1) % NEIGHBOR_HISTORY_SIZE) {
      if (it.value()->all_ett[i] != 0) {
        ett_window += it.value()->all_ett[i]; 
	count_window++;
      }
    }
    it.value()->avg_sett = ett_window / count_window;

    /* update long term ett over link state history */
    ett_window = 0;
    count_window = 0;
    for (int i = 0; i < NEIGHBOR_HISTORY_SIZE; ++i) {	
      if (it.value()->all_ett[i] != 0) {
        ett_window += it.value()->all_ett[i]; 
        count_window++;
      }
    }
    
    it.value()->l_ett = ett_window / count_window;

    it.value()->expiry->schedule_after_msec(NEIGHBOR_HEARTBEAT_TIMEOUT_MSECS);
    logger.log(MF_INFO, 
      "nbr_tbl: lp-stat guid: %u ett: %u seqno: %u sett: %u lett: %u",
      nbr_guid, sett_rcvd, seqno, it.value()->avg_sett, it.value()->l_ett);

    it.value()->prev_seq_no = seqno;
  } else {  // else create a new entry 
    neighbor_t *nbr = (neighbor_t*)malloc(sizeof(neighbor_t));
    
    nbr->guid = nbr_guid;
    nbr->s_ett = sett_rcvd;
    for (int j = 0; j < NEIGHBOR_HISTORY_SIZE; j++) {
      nbr->all_ett[j] = 0;
    }
    nbr->all_ett[seqno % NEIGHBOR_HISTORY_SIZE] = sett_rcvd;
    nbr->avg_sett = sett_rcvd;
    nbr->l_ett = sett_rcvd;
    nbr->prev_seq_no = seqno;
		
    timer_data_t *timerdata = new timer_data_t();
    timerdata->guid = nbr_guid;
    timerdata->neighbor_tbl = this;
    nbr->expiry = new Timer(&MF_NeighborTable::handleExpiry,timerdata);
    nbr->expiry->initialize(this);
    nbr->expiry->schedule_after_msec(NEIGHBOR_HEARTBEAT_TIMEOUT_MSECS);

    neighbors->set(nbr_guid, nbr);

    logger.log(MF_TIME,
      "nbr_tbl: New Neighbor! guid: %u ", nbr_guid);

    logger.log(MF_INFO, 
      "nbr_tbl: lp-stat guid: %u ett: %u seqno: %u sett: %u lett: %u",
      nbr_guid, sett_rcvd, seqno, nbr->avg_sett, nbr->l_ett);
  }
  return true;
}

bool MF_NeighborTable::remove(uint32_t guid){
  neighbor_map_t::iterator it = neighbors->find(guid);
  if (it != neighbors->end()) {          //if exists
    delete it.value()->expiry;
    delete it.value();
    neighbors->erase(it);
    return true; 
  }
  return false;
}

uint32_t MF_NeighborTable::size(){
  return neighbors->size();
}

bool MF_NeighborTable::get_neighbor(uint32_t guid, neighbor_t &nbr){
  
  neighbor_map_t::iterator it = neighbors->find(guid);
  if (it != neighbors->end()) {  //if entry guid exists, 
    nbr = *(it.value());
    return true; 
  }
  return false;
}

/* Populates provided vector with current neighbors' information */

void MF_NeighborTable::get_neighbors(Vector<const neighbor_t*> &nbrs_v){
  for (neighbor_map_t::const_iterator iter = neighbors->begin(); 
        iter != neighbors->end(); ++iter) {
    nbrs_v.push_back(iter.value());
  }
}

//fires when neighbor heartbeat timer expires

void MF_NeighborTable::handleExpiry(Timer*, void * data){
  timer_data_t * timerdata = (timer_data_t*) data;
  assert(timerdata);
  timerdata->neighbor_tbl->expire(timerdata->guid, timerdata);
}

void MF_NeighborTable::expire(const uint32_t guid, timer_data_t * timerdata){
  neighbor_map_t::iterator it = neighbors->find(guid);
  if (it != neighbors->end()) {
    delete it.value()->expiry;
    delete it.value(); 
    neighbors->erase(it);
    logger.log(MF_TIME, 
      "nbr_tbl: Timer expires, DELETING guid: %u", guid);
    delete timerdata;
  } else {
    logger.log(MF_ERROR,
      "nbr_tbl: Cannot find target guid: %u entry to delete", guid); 
  }
  /* signal for forward table to be recomputed */
  /* TODO: schedule a event rather than call compute func */	
}

void MF_NeighborTable::print(){
  logger.log(MF_INFO, "nbr_tbl: Neighbor table:");
  for (neighbor_map_t::const_iterator iter = neighbors->begin(); 
         iter != neighbors->end(); ++iter){
    uint32_t guid = iter.key();
    neighbor_t *nbr = iter.value();
    logger.log(MF_INFO, "nbr_tbl: %u %d %d",guid, nbr->s_ett, nbr->avg_sett);
  }
}

String MF_NeighborTable::read_handler(Element *e, void *thunk) {
  
  MF_NeighborTable *nt = (MF_NeighborTable *)e;
  StringAccum accum_str;
  String resp;

  //TODO: KN: synchronize access to NT
  switch ((intptr_t)thunk) {
  case 1:
    for (neighbor_map_t::const_iterator iter = nt->neighbors->begin();
           iter !=  nt->neighbors->end(); ++iter){
      neighbor_t *nbr = iter.value();
      uint32_t guid = iter.key();
      String str1 = String(guid);
      accum_str << "[" << str1 << ",";

      String str2 = String(nbr->s_ett);
      accum_str << str2 << ",";
			
      String str3 = String(nbr->l_ett);
      accum_str << str3 << "],";
    }
  default:
    accum_str << "[]";	
  }

  resp = accum_str.take_string();
  return resp;
}

void MF_NeighborTable::add_handlers() {
  add_read_handler("neighbor_table", read_handler, (void *)1);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_NeighborTable)
