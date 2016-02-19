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
#ifndef NEIGHBORTABLE_HH
#define NEIGHBORTABLE_HH

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/timer.hh>
#include "mf.hh"
#include "mflogger.hh"



CLICK_DECLS

/* TODO: KN: This currently determines 'long term' in LETT 
 * but perhaps the window used to estimate LETT should be 
 * link-type specific? Access links (w/ mobile client), 
 * wired links, and fixed wireless mesh links deserve 
 * separate consideration.
 */
#define NEIGHBOR_HISTORY_SIZE 10

/* window over which ett values are averaged to determine SETT */
#define SETT_WINDOW_SIZE 3

typedef struct neighbor {
  uint32_t guid;
  time_t s_ett;
  time_t avg_sett;
  time_t all_ett[NEIGHBOR_HISTORY_SIZE];
  time_t l_ett;
  Timer* expiry;
  uint32_t prev_seq_no;
} neighbor_t;

typedef HashTable<uint32_t, neighbor_t*> neighbor_map_t;

class MF_NeighborTable : public Element {
public:
  MF_NeighborTable();
  ~MF_NeighborTable();

  const char *class_name() const	{ return "MF_NeighborTable"; }
  const char *port_count() const	{ return PORTS_0_0; }
  const char *processing() const	{ return AGNOSTIC; }
  int configure(Vector<String> &, ErrorHandler *);
  void add_handlers();

  /* functions */
	
  /* pretty print of neighbor table */
  void print();

  /* insert (update if exists) neighbor and link state */
  bool insert(uint32_t guid, time_t sett_rcvd, uint32_t seqno);
  /* remove specified neighbor */
  bool remove(uint32_t guid);
  /* number of neighbors */
  uint32_t size();
  /* neighbor and link state info */
  bool get_neighbor(uint32_t guid, neighbor_t &);
  /* dump of current neighbors and their link states */
  void get_neighbors(Vector<const neighbor_t*> &);

private:
  neighbor_map_t *neighbors;

  // callback function for timers
  static void handleExpiry(Timer*, void *);
  typedef struct TimerData{
    MF_NeighborTable* neighbor_tbl;
    uint32_t guid;
  } timer_data_t;  
  void expire(const uint32_t, timer_data_t *);

  static String read_handler(Element *e, void *thunk);

  MF_Logger logger;
};

CLICK_ENDDECLS;

#endif /* NEIGHBORTABLE_HH */
