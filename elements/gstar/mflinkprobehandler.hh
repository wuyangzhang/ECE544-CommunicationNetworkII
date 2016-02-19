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
#ifndef MF_LINKPROBEHANDLER_HH
#define MF_LINKPROBEHANDLER_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/etheraddress.hh>
#include "mf.hh"
#include "mflogger.hh"
#include "mfneighbortable.hh"
#include "mfroutingtable.hh"
#include "mfarptable.hh"


CLICK_DECLS

#define MAX_SEQ_NUM 10

#define LINK_PROBE_HISTORY_SIZE 10

/* struct maintains history of probes sent out -- timestamps
 * index is identified by sequence number modulo history size
 * TODO: create an entry struct and define an array of these
 */
struct link_probe_history{
	/* timestamp when lp was sent, indexed by seq_no */
	struct timeval ts[LINK_PROBE_HISTORY_SIZE];
};

/**
 * Link Probe Messages Handler
 *
 * Sends periodical broadcast probes to discover new neihgbors and
 * test link conditions to exiting neighbors
 * 
 * Message Type on Ports:
 * Input:  	port 0: Link probe from neighbor
 * 		port 1: Link probe acknowledgement from neighbor
 *
 * Output: 	port 0: Link probe broadcast
 * 		port 1: Link probe acknowledgement to neighbor
 *
 * Timer: To periodically broadcast a link probe message
 */
class MF_LinkProbeHandler: public Element{
public:
  MF_LinkProbeHandler();
  ~MF_LinkProbeHandler();

  const char *class_name() const	{return "MF_LinkProbeHandler";}
  const char *port_count() const	{ return "2/2"; }
  const char *processing() const	{ return PUSH; }
  
  void push (int, Packet *);
  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void run_timer(Timer *);

private:
  /* timer for periodic link probe broadcasts */
  Timer lp_timer;

  /* a monotonically increasing identifer for link probes */
  uint32_t _seq_no;

  struct link_probe_history lp_hist;

  /* The followed are passed during element init */
  uint32_t my_guid;
                
  /* 
   * Reference to neighbor table element to perform new
   * neighbor inserts on receipt of acks to link probe
   * broadcasts
   */
  MF_NeighborTable* neighbor_tbl_p;
  MF_RoutingTable * routing_tbl_p;

  /* funcs */
  void handleLinkProbePkt(Packet *);
  void handleLinkProbeAckPkt(Packet *);
  void save_lp_timestamp(uint32_t, struct timeval);
  struct timeval get_lp_timestamp(uint32_t);

  MF_Logger logger;

};

CLICK_ENDDECLS;

#endif //MF_LINKPROBEHANDLER_HH
