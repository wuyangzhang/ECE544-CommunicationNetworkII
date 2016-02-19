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
#ifndef MF_ROUTING_HH
#define MF_ROUTING_HH

#include <click/element.hh>
#include <click/task.hh>
#include <click/hashtable.hh>
#include <click/timer.hh>

#include <pthread.h>

#include "mfneighbortable.hh"

/*
 * MF_RoutingTable(MY_GUID,NEIGHBOR_TABLE)
 * MY_GUID : my ID
 * NEIGHBOR_TABLE : to get the neighbor information for 
 * creating the forward table
 *
 * No Input/Output
 * information element
 *
 * saves the received LSAs in lsa_table struct 
 * saves the forward table in forward struct
*/
CLICK_DECLS

/* max weight to initialize path costs in Dijsktra's alg */
/* if metric is ETT in usecs, then 10M is 10 seconds */
#define MAX_WEIGHT 10000000

/* forwarding table recompute interval */
#define FT_UPDATE_PERIOD_MSECS 1000

/* force recompute of FT after max skips */
#define MAX_FT_SKIPPED_UPDATES 2

/* number of epochs after which a un-updated FT entry is stale */
#define MAX_FT_EPOCH_STALENESS 2 

/* max number of nodes in path */
#define MAX_PATH_LENGTH 50

/* normal range in the store or forward decision space */
/* ett factor = short_term_ett/long_term_ett */
#define ETT_FACTOR_LOWER 0.9
#define ETT_FACTOR_UPPER 1.1

/* Store or forward decision returned based on path and link state*/
#define RTG_DECISION_STORE 0
#define RTG_DECISION_FORWARD 1
#define RTG_NO_ENTRY -1

typedef struct lsa_table {
  int seq_no;
  int len;
  uint32_t *ip_n;
  uint32_t *sett;
  uint32_t *lett;
  Timer* expiry;			
} lsa_tbl_t;

typedef struct forward {
  uint32_t weight;
  uint32_t lett_sum;
  uint32_t nexthop;
  uint32_t path[MAX_PATH_LENGTH];
  int path_length;
  uint32_t epoch;
} forward_t;

typedef struct tentative_history {
  uint32_t tent_wt;
  uint32_t tent_lett_sum;
  uint32_t tent_nexthop;
  uint32_t tent_path[MAX_PATH_LENGTH];
  int tent_path_length;
} tentative_history_t;

typedef HashTable<uint32_t, tentative_history_t*> tent_map;
typedef HashTable<uint32_t, lsa_tbl_t*> LSA_map;
typedef HashTable<uint32_t, forward_t*> forward_map;

class MF_RoutingTable : public Element{ 
public:
  MF_RoutingTable();
  ~MF_RoutingTable();
		
  const char *class_name() const	{ return "MF_RoutingTable"; }
  const char* port_count() const	{ return PORTS_0_0; }
  const char *processing() const	{ return AGNOSTIC; }
  MF_RoutingTable *clone() const	{ return new MF_RoutingTable; }
		
  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void run_timer(Timer *);

  int insertLSA(Packet *);

  uint32_t getNextHopGUID(uint32_t);
  int get_forward_flag(uint32_t);

  uint32_t getWeight(uint32_t dst_GUID);
  /* 
   * Returns the length in number of hops to the dst 
   * node specified by GUID. Returns -1 if no
   * path to destination.
   */
  int get_path_length(uint32_t dst_GUID);

  MF_NeighborTable * getNeighborTable() {
    return neighbor_tbl_p;
  }

  forward_map * getForwardTable() {
    return forward_table;
  } 

  LSA_map * getLSAMap() {
    return lsa_all;
  } 
 
  /* to request update to FT when routing state changes */
  void req_ft_update();
				
private:
  /* timer for periodic FT rebuilds */
  Timer ft_update_timer;

  /* set to true by modules handling routing state update 
   * indicating FT rebuild is required 
   */
  bool ft_update_flag;
  unsigned ft_skipped_updates;

  uint32_t my_guid;
  uint32_t ft_epoch; //epoch to mark recompute iteration of forwarding table
		
  LSA_map *lsa_all;

  //uint32_t lett_cost_dest3,sett_cost_dest3;
  //int forward_flag_dest3;
		
  //forward_map forward_table;
  forward_map* forward_table;	

  typedef struct TimerData{
    MF_RoutingTable* routing_tbl;
    uint32_t guid;
  } timer_data_t;
  // callback function for timers
  static void handleExpiry(Timer*, void *); 
  void expire(const uint32_t, timer_data_t *);
  void forwardtable();
  MF_NeighborTable * neighbor_tbl_p;

  /* Synchronization for computing forwarding table.
   * Currently timer expirations, LSA message 
   * arrivals and neighbor table changes trigger
   * routing/forwarding table recalculation.	
   * TODO: separate forwarding table from routing table;
   * Create periodic FT computation timer, 
   * and queue any asynchronous recomputation requests.
   */
  pthread_mutex_t _ft_lock;

  /* pretty print the forwarding table */
  void print_ft();

  MF_Logger logger;
};

CLICK_ENDDECLS
#endif //MF_ROUTING_HH

