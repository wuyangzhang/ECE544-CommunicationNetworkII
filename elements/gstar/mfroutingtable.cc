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
#include <click/args.hh>
#include <click/handlercall.hh>
#include <pthread.h>

#include "mfroutingtable.hh"
#include "mffunctions.hh"
#include "mflogger.hh"


CLICK_DECLS
MF_RoutingTable::MF_RoutingTable(): ft_update_timer(this), 
                   ft_update_flag(false), ft_skipped_updates(0), 
                   logger(MF_Logger::init()){
  forward_table = new forward_map();
  lsa_all = new LSA_map(); 
  pthread_mutex_init(&_ft_lock, NULL);
}

MF_RoutingTable::~MF_RoutingTable(){

}

int MF_RoutingTable::configure(Vector<String> &conf, ErrorHandler *errh){
  if (cp_va_kparse(conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpInteger, &my_guid,
        "NEIGHBOR_TABLE", cpkP+cpkM, cpElement, &neighbor_tbl_p,
        cpEnd) < 0) {
    return -1;
  }
  return 0; 
}

int MF_RoutingTable::initialize(ErrorHandler *){
  ft_update_timer.initialize(this);
  ft_update_timer.schedule_after_msec(FT_UPDATE_PERIOD_MSECS);
  return 0;
}

/**
 * Timer Task handler
 *
 * Rebuild forwarding table from current routing state
 * 
 */
void MF_RoutingTable::run_timer(Timer *){
  /* 
   * Check and skip if there were no updates to routing state 
   * and no rebuilding of the forward table was requested 
   */
  pthread_mutex_lock(&_ft_lock);
  if (ft_update_flag || ft_skipped_updates >= MAX_FT_SKIPPED_UPDATES) {
    ft_update_flag = false;
    ft_skipped_updates = 0;
    pthread_mutex_unlock(&_ft_lock);
    forwardtable();
    //DEBUG
    print_ft();
  } else {
    ft_skipped_updates++;
    pthread_mutex_unlock(&_ft_lock);
  }
  ft_update_timer.schedule_after_msec(FT_UPDATE_PERIOD_MSECS);
}

/**
  * Request forward table rebuild 
  *
  * Sets flag read by periodic timer task that recomputes the 
  * forwarding table if the flag is set.
  */
void MF_RoutingTable::req_ft_update(){
  pthread_mutex_lock(&_ft_lock);
  ft_update_flag = true;
  pthread_mutex_unlock(&_ft_lock);
}

//functions to delete the history of node after LSA Timer msec

void MF_RoutingTable::handleExpiry(Timer*, void * data){
  timer_data_t * timerdata = (timer_data_t*) data;
  assert(timerdata);
  timerdata->routing_tbl->expire(timerdata->guid, timerdata);
}

void MF_RoutingTable::expire(const uint32_t guid, timer_data_t * timerdata){
  LSA_map::iterator it = lsa_all->find(guid); 
  // pass timerdata too to clean up memory after timer expires completely
  if (it != lsa_all->end()) {
    logger.log(MF_INFO, 
      "rtg_tbl: Removing LS history of node guid %u -- LSA timer expired\n", 
       guid);
    delete it.value()->expiry;
    delete it.value();
    lsa_all->erase(it);
    delete timerdata; 
    //call for recompute of forward table
    req_ft_update(); 
  } 
}

// saving the received LSA
int MF_RoutingTable::insertLSA(Packet * packet){

  int lsa_flag = 0;
  int update = 0;
			
  lsa_t * header = (lsa_t*)packet->data();
	
  // get the source addr in LSA
  uint32_t src_guid = ntohl(header->sourceLSA);		

  // check to see if their is already a LSA stored
  LSA_map::iterator it = lsa_all->find(src_guid);
  if (it != lsa_all->end()) {
    uint32_t seq_no_rcvd = ntohl(header->seq);
    uint32_t seq_no_prev = it.value()->seq_no;
    //check to see if the sequence no of the LSA received 
    //is greater than that of the LSA already saved 
    if (seq_no_rcvd > seq_no_prev) {			
			
      delete [] it.value()->ip_n;
      delete [] it.value()->sett;
      delete [] it.value()->lett; 
      delete it.value()->expiry;
      delete it.value();
      lsa_all->erase(it); 
      update = 1;
    }
  } else {
    update = 1;
  }

  //if the LSA is new, save it in the database
  if (update == 1) {
    uint32_t size = ntohl(header->size); // no of neighbors in the LSA

    lsa_tbl_t *tmp = (lsa_tbl_t*)malloc(sizeof(lsa_table));
    assert(tmp);

    tmp->len = size;                     // save the no of neighbor info
    tmp->seq_no = ntohl(header->seq); 
		
    // saving the setts of the neighbors in the LSA
    tmp->sett = (uint32_t*)malloc(size*sizeof(uint32_t));
    assert(tmp->sett);
    uint32_t * sett_lsa = (uint32_t *) (header + 1);

    for (uint32_t i = 0; i < size; ++i) {
      //tmp->sett[i] = *sett_lsa;
      //sett_lsa++;
      tmp->sett[i] = *sett_lsa++; 
    }
		
    // saving the letts of the neighbors in the LSA
    tmp->lett = (uint32_t*)malloc(size*sizeof(uint32_t));
    uint32_t * lett_lsa = (uint32_t *)sett_lsa;

    for (int i = 0; i < size; i++) {
      //tmp->lett[i] = *lett_lsa;
      //lett_lsa++;
      tmp->lett[i] = *lett_lsa++; 
    }
	
    // saving the IDS of the neighbors in the LSA
    tmp->ip_n = (uint32_t*)malloc(size*sizeof(uint32_t));
    uint32_t * address = (uint32_t *) (lett_lsa);

    for (int i = 0; i < size; i++) {
      //tmp->ip_n[i] = *address;
      //address++;
      tmp->ip_n[i] = *address++; 
    }

    // initilizing timer
    timer_data_t *timerdata = new timer_data_t(); 		
    timerdata->guid = src_guid;
    timerdata->routing_tbl = this;
    tmp->expiry = new Timer(&MF_RoutingTable::handleExpiry, timerdata);
    tmp->expiry->initialize(this);
    tmp->expiry->schedule_after_msec(LSA_LIVENESS_TIMEOUT_MSECS);

    lsa_all->set(src_guid, tmp);
    
    //TODO: assess any real change in LS, not just that a new LSA
    //was received. This is resulting in too many FT recomputes!
    lsa_flag = 1;
  }

  //recompute the forward table, if there is a change in the lsa table
  if (lsa_flag == 1) {
    //call for recompute of forward table
    req_ft_update();
  }
  return lsa_flag;
}

void MF_RoutingTable::forwardtable(){

  uint32_t lett, sett;
  uint32_t address;
  uint32_t min_wt = MAX_WEIGHT;       // the maximum the uint32_t can support
  uint32_t dest_guid;
  uint32_t next_hop_guid = 0;
  uint32_t path_ip[MAX_PATH_LENGTH];
  int pl = 0;
  tent_map *tentative_table = new tent_map();
	
  uint32_t confirmed[MAX_PATH_LENGTH];	
  int size_confirmed = 0;


  //increment forwarding table epoch	
  ft_epoch++;
  logger.log(MF_DEBUG, 
    "rtg_tbl: forwardtable being recomputed; epoch: %u", ft_epoch);

  Vector<const neighbor_t*> nbrs_vec;

  neighbor_tbl_p->get_neighbors(nbrs_vec);
  int length = nbrs_vec.size();

  //forward_table = new forward_map();	
  //Dijsktra's Algorithm (Forward Search)
  for (uint32_t i = 0; i < length; ++i) {
    const neighbor_t *nbr = nbrs_vec[i];
    sett = nbr->s_ett;
    lett = nbr->l_ett;
    min_wt = sett;
    address = nbr->guid;
    dest_guid = address;
    next_hop_guid = address;
		
    tentative_history_t *tent_entry = 
      (tentative_history_t*)malloc(sizeof(tentative_history_t));
		
    tent_entry->tent_wt = min_wt;
    tent_entry->tent_lett_sum = lett;
    tent_entry->tent_nexthop = next_hop_guid;
    tent_entry->tent_path[0] = next_hop_guid;
    tent_entry->tent_path_length = 1;
	
    tentative_table->set(dest_guid, tent_entry);
  }
	
  min_wt = MAX_WEIGHT;
  uint32_t wt, lett_curr = MAX_WEIGHT;
  while (tentative_table->size() > 0) {
    for (tent_map::const_iterator iter = tentative_table->begin(); 
                     iter != tentative_table->end(); ++iter) {
      wt = iter.value()->tent_wt;
      if (min_wt > wt) {
        min_wt = wt;
        lett_curr = iter.value()->tent_lett_sum;
        dest_guid = iter.key();
        next_hop_guid = iter.value()->tent_nexthop;
        pl = iter.value()->tent_path_length;
        for (uint32_t m = 0; m < pl; ++m) {
          path_ip[m] = iter.value()->tent_path[m];
        }
      }
    }
    tentative_table->erase(dest_guid); 

    int skip1 = 0;
    int skip2 = 0;

    //struct forward entry;
    forward_t *entry = (forward_t*)malloc(sizeof(forward_t)); 
	
    entry->weight = min_wt;
    entry->lett_sum = lett_curr;
    entry->nexthop = next_hop_guid;
    entry->path_length = pl;
    entry->epoch = ft_epoch;

    for (uint32_t i = 0; i < pl; ++i) {
      entry->path[i] = path_ip[i];
    }

    forward_table->set(dest_guid,entry);
		
    confirmed[size_confirmed] = dest_guid;
    size_confirmed++;
    LSA_map::iterator it = lsa_all->find(dest_guid);
    if (it != lsa_all->end()) { 
      int length = it.value()->len;
      for (uint32_t i = 0; i < length; ++i) {
	skip1 = 0;
        skip2 = 0;
        tentative_history_t *tent_entry = 
          (tentative_history_t*)malloc(sizeof(tentative_history_t)); 
        tent_entry->tent_wt = it.value()->sett[i] + min_wt; 
        tent_entry->tent_lett_sum = it.value()->lett[i] + lett_curr; 
        tent_entry->tent_nexthop = next_hop_guid; 
        uint32_t destination = it.value()->ip_n[i]; 
        tent_entry->tent_path_length = pl + 1; 
        for (uint32_t m = 0; m < pl; ++m) {
          tent_entry->tent_path[m + 1] = path_ip[m]; 
        }
        tent_entry->tent_path[0] = destination; 

        for (uint32_t j = 0; j < size_confirmed; ++j) {
          if (destination == confirmed[j]) {
            skip1 = 1;
          }
        }

        for (tent_map::const_iterator iter0 = tentative_table->begin(); 
                        iter0 != tentative_table->end(); ++iter0) {
          if (destination == iter0.key()) {
            if (tent_entry->tent_wt > iter0.value()->tent_wt) {
              skip2 = 1;
            } else {
              tentative_table->erase(destination); 
              break;
            }
          }
        }
        
        if (skip1 == 0 && skip2 == 0 && destination != my_guid) {
          tentative_table->set(destination, tent_entry); 
        }
      }
    }
    min_wt = MAX_WEIGHT; 
  }
  /*	
  if (forward_map::Pair* pair = forward_table->find_pair(3)) {
    lett_cost_dest3 = pair->value.lett_sum;
    sett_cost_dest3 =   pair->value.weight;
    forward_flag_dest3 = get_forward_flag(3);
  }*/
}

void MF_RoutingTable::print_ft(){
  logger.log(MF_DEBUG, "rtg_tbl: ######### Forward Table #########");
  for (forward_map::const_iterator iter1 = forward_table->begin(); 
                     iter1 != forward_table->end(); ++iter1) {
    logger.log(MF_DEBUG, 
      "rtg_tbl: FT entry: dst=%d, nexthop=%d, pathlength=%d, epoch=%d",
      iter1.key(), iter1.value()->nexthop, iter1.value()->path_length,
      iter1.value()->epoch);
  }
}

uint32_t MF_RoutingTable::getNextHopGUID(uint32_t dst_GUID) {

  uint32_t nextHopGUID = 0; 
  forward_map::iterator it = forward_table->find(dst_GUID); 
  if (it != forward_table->end()) {
    /* check if entry is stale, i.e., the epoch in which it was 
     * added is older than max allowable. If so, entry
     * should be removed. Call for FT update 
     */
    if ((ft_epoch - it.value()->epoch) >= MAX_FT_EPOCH_STALENESS) {
      logger.log(MF_INFO, 
        "rtg_tbl: No next hop -- stale entry for guid %u "
        "(epoch now: %u, entry epoch: %u)", 
      dst_GUID, ft_epoch, it.value()->epoch);
      req_ft_update();
    } else {
      nextHopGUID = it.value()->nexthop;
    }
  }
  return nextHopGUID;
}

int MF_RoutingTable::get_forward_flag(uint32_t dst_guid){
  
  uint32_t path_lett; 
  uint32_t path_sett;
  uint32_t next_hop_guid;
  
  forward_map::iterator it = forward_table->find(dst_guid);
  if (it != forward_table->end()) { 
    next_hop_guid = it.value()->nexthop;
    path_lett = it.value()->lett_sum;
    path_sett = it.value()->weight;
  } else {
    /* no entry, hold & rebind at suitable time */
    //return RTG_DECISION_STORE;
    return RTG_NO_ENTRY; 
  }
  
  /* Forward if both path state and the link state to the next hop is
   * favorable right now */
  neighbor_t nbr;
  if (!neighbor_tbl_p->get_neighbor(next_hop_guid, nbr)) {
    /* neighbor has vanished, next hop needs rebinding */
    /* technically not an error, but shouldn't be common */
    logger.log(MF_INFO, 
      "rtg_tbl: next hop guid: %u vanished from neighbor tbl "
      "during route lookup",next_hop_guid);
    /* TODO: KN: should the entry be deleted/marked stale? */
    return RTG_DECISION_STORE;
  }
  
  if ((path_sett < (ETT_FACTOR_UPPER * path_lett)) 
         && (nbr.s_ett < (ETT_FACTOR_UPPER * nbr.l_ett))){
    return RTG_DECISION_FORWARD;
  }
 
  /* Sub-normal path quality. Hold and rebind later */
  logger.log(MF_DEBUG, 
    "rtg_tbl: STORE. dst: %u nxthop: %u path ett s/l: %u/%u "
    "nbr ett s/l: %u/%u",
    dst_guid, next_hop_guid, path_sett, path_lett, nbr.s_ett, nbr.l_ett);
  return RTG_DECISION_FORWARD;
}

int MF_RoutingTable::get_path_length(uint32_t dst_guid) {
  
  int path_length = -1;                //indicates no path to dst 
  forward_map::iterator it = forward_table->find(dst_guid);
  if (it != forward_table->end()) {
    /* check if entry is stale, i.e., the epoch in which it was 
     * added is older than max allowable. If so, entry
     * should be removed. Call for FT update 
     */
    if ((ft_epoch - it.value()->epoch) >= MAX_FT_EPOCH_STALENESS) {
      logger.log(MF_INFO, 
        "rtg_tbl: No path -- stale entry for guid %u "
        "(epoch now: %u, entry epoch: %u)", 
	dst_guid, ft_epoch, it.value()->epoch);
      req_ft_update();
    } else {
      path_length = it.value()->path_length;
    }
  }
  return path_length;
}
	
CLICK_ENDDECLS

EXPORT_ELEMENT(MF_RoutingTable)

