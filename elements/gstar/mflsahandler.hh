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
#ifndef LSAHANDLER_HH
#define LSAHANDLER_HH

#include <click/element.hh>
#include <click/timer.hh>
#include "mf.hh"
#include "mflogger.hh"
#include "mfneighbortable.hh"
#include "mfroutingtable.hh"

CLICK_DECLS

/**
 * Link-State Advertisements (LSAs) Handler
 * 
 * Helps implement link state routing protocol by periodically 
 * distributing local node's observations on link state with it's
 * neighbors and also to record and forward (re-broadcast) any LSAs
 * heard from it's neighbors.
 * 
 * Message Type on Ports:
 * Input:  	port 0: LSA from neighbor
 *
 * Output: 	port 0: LSA (own or neighbor) for broadcast
 *
 * Timer: To periodically construct & broadcast LSA of local node
 */ 
class MF_LSAHandler: public Element{
public:
  MF_LSAHandler();
  ~MF_LSAHandler();

  const char *class_name() const	{ return "MF_LSAHandler"; }
  const char *port_count() const	{ return "1/1"; }
  const char *processing() const	{ return PUSH; }
		
  void push (int, Packet *);
  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void run_timer(Timer *);

private:
  uint32_t _seq_no;
  Timer timer;

  /* The followed are passed during element init */
  uint32_t my_guid;
  MF_NeighborTable * neighbor_tbl_p;
  MF_RoutingTable * routing_tbl_p;

  /* functions */
  void handleLSA(Packet *);

  MF_Logger logger;
};

CLICK_ENDDECLS;

#endif //LSAHANDLER_HH
