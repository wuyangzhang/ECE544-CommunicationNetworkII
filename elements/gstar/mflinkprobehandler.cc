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
#include <click/timestamp.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "mflinkprobehandler.hh"
#include "mfneighbortable.hh"
#include "mfroutingtable.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_LinkProbeHandler::MF_LinkProbeHandler(): lp_timer(this), _seq_no(0), 
                                            logger(MF_Logger::init()){
}

MF_LinkProbeHandler::~MF_LinkProbeHandler(){

}

int MF_LinkProbeHandler::configure(Vector<String> &conf, ErrorHandler *errh){
  if (cp_va_kparse(conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpInteger, &my_guid,
        "NEIGHBOR_TABLE", cpkP+cpkM, cpElement, &neighbor_tbl_p,
        "ROUTING_TABLE", cpkP+cpkM, cpElement, &routing_tbl_p,
        cpEnd) < 0) {
    return -1;  
  }
  return 0; 
}

int MF_LinkProbeHandler::initialize(ErrorHandler *){
  lp_timer.initialize(this);
  lp_timer.schedule_after_msec(LINKPROBE_PERIOD_MSECS);
  return 0;
}

/**
 * Timer Task handler.
 * Sends a broadcast link probe to discover new neihgbors and to estimate
 * link qualities to neighbors by way of their acks.
 */
void MF_LinkProbeHandler::run_timer(Timer *){

  // creating link probe packet
  int headroom = sizeof(click_ether) + sizeof(click_ip);
  WritablePacket *packet = Packet::make(headroom, 0, sizeof(link_probe_t), 0);
	  
  if (!packet) {
    logger.log(MF_CRITICAL,"link_probe: can't make probe packet!");
    return;
  }
  memset(packet->data(), 0, packet->length());

  //inserting link probe packet data	
  link_probe_t * lp = (link_probe_t *) packet->data();
       
  lp->type = htonl(LP_PKT); 
  lp->sourceLP = htonl(my_guid); 
  lp->destinationLP = htonl(BROADCAST_GUID); //broadcast
  lp->seq = htonl(_seq_no);

  struct timeval ts;
  gettimeofday(&ts, NULL); 
  //sending link probe packet	
  logger.log(MF_DEBUG, 
    "link_probe: Sending link probe, seq no.: %u", _seq_no);

  packet->set_anno_u32(NXTHOP_ANNO_OFFSET, BROADCAST_GUID);

  output(0).push(packet);

  //saving timestamp & seq no.
  save_lp_timestamp(_seq_no, ts);
  //reschedule timer 
  lp_timer.schedule_after_msec(LINKPROBE_PERIOD_MSECS);

  //increment sequence number
  _seq_no = (_seq_no + 1) % MAX_SEQ_NUM;
	
}

void MF_LinkProbeHandler::push(int port, Packet * pkt){
  if (port == 0) {
    /* link probe message from neighbor */
    handleLinkProbePkt(pkt);
  } else if (port == 1) {
    /* link probe ACK from neighbor */
    handleLinkProbeAckPkt(pkt);
  } else {
    logger.log(MF_FATAL, "link_probe: Packet received on unsupported port");
    exit(-1);
  }
}

void MF_LinkProbeHandler::handleLinkProbePkt(Packet * pkt){

  link_probe_t * lp = (link_probe_t*) pkt->data();
	
  uint32_t src_guid = ntohl(lp->sourceLP); 
  if (src_guid == my_guid) {
    //pkt originated at this node
    pkt->kill();
    return;
  }

  logger.log(MF_DEBUG, 
    "link_probe: Recvd link probe from guid: %u", src_guid);

  //creating link probe ACK packet	
  int headroom = sizeof(click_ether) + sizeof(click_ip);  	
  //fixed size pkt, no tailroom required
  WritablePacket *ack_pkt = 
                    Packet::make(headroom, 0, sizeof(link_probe_ack_t), 0);
  if (!ack_pkt) {
    logger.log(MF_CRITICAL, "link_probe: can't make probe ack packet!");
    pkt->kill();
    return;
  }
  memset(ack_pkt->data(), 0, ack_pkt->length());
  //prepare the LP ack packet
  link_probe_ack_t * lp_ack = (link_probe_ack_t *)ack_pkt->data();

  lp_ack->type = htonl(LP_ACK_PKT); 
  lp_ack->sourceLPACK = htonl(my_guid); 
  lp_ack->destinationLPACK = htonl(src_guid);
  lp_ack->seq_no_cp = lp->seq; // seq no from original LP msg

  //ack_pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, BROADCAST_GUID);
  ack_pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, src_guid); 

  logger.log(MF_DEBUG, 
    "link_probe: sending lp ack to guid: %u via bcast", src_guid);
  output(1).push(ack_pkt);
  
  pkt->kill();
}

void MF_LinkProbeHandler::handleLinkProbeAckPkt(Packet* pkt){

  link_probe_ack_t * header = (link_probe_ack_t*) pkt->data();
  uint32_t src_guid = ntohl(header->sourceLPACK); //neighbor ID
  uint32_t dest_guid = ntohl(header->destinationLPACK); 

  if (dest_guid != my_guid) {
    //shouldn't happen if acks are unicast
    logger.log(MF_ERROR, 
      "link_probe: recvd probe ack addressed to node with guid: %u!", 
      dest_guid);
    pkt->kill();
    return;
  }

  logger.log(MF_DEBUG, 
    "link_probe: Recvd link probe ack from guid: %u", src_guid);

  uint32_t ack_seq_no = ntohl(header->seq_no_cp); 
	
  // recover time (msec) at which the LP was sent
  struct timeval ts = get_lp_timestamp(ack_seq_no);
	
  struct timeval curr_ts;
	
  /*
   * If packet has time stamp annotation, then use that
   * or else, get timestamp now
   */ 
	
  const Timestamp & arr_time = pkt->timestamp_anno();

  curr_ts.tv_sec = arr_time.sec();
  curr_ts.tv_usec = arr_time.usec();
  if (!(curr_ts.tv_sec + curr_ts.tv_usec)) {
    //no ts anno, get current system time
    gettimeofday(&curr_ts, NULL); 
  }

  time_t dif_sec = curr_ts.tv_sec - ts.tv_sec;
  time_t dif_usec = curr_ts.tv_usec - ts.tv_usec;
	
  if (dif_usec < 0){
    dif_sec--, 
    dif_usec = dif_usec +  1000000;
  }
	
  time_t rtt = dif_sec * 1000000 + dif_usec;

  // calculating SETT
  //TODO: currently approximated as 0.5*rtt
  time_t sett = 0.5 * rtt;

  logger.log(MF_DEBUG, 
    "link_probe: guid: %u seqno: %u SETT: %u",
    src_guid, ack_seq_no, sett);

  /* check if this will be new entry in neighbor table 
   * If yes, then request for rebuild of forward table 
   */

  neighbor_t nbr;
  
  bool rebuild_FT = false;
  
  if (!neighbor_tbl_p->get_neighbor(src_guid, nbr)) {
    rebuild_FT = true;		
  }
	
  /* insert into neighbor table  -- this updates if already present */
  neighbor_tbl_p->insert(src_guid, sett, ack_seq_no);
  if(rebuild_FT)routing_tbl_p->req_ft_update();
  pkt->kill();
}


// saves the timestamp at which the link probe was sent 
void MF_LinkProbeHandler::save_lp_timestamp(uint32_t seq_no, struct timeval ts){
	
  lp_hist.ts[seq_no % LINK_PROBE_HISTORY_SIZE] = ts;
}

//returns timestamp (sec) when specified probe was sent
struct timeval MF_LinkProbeHandler::get_lp_timestamp(uint32_t seq_no){
  return lp_hist.ts[seq_no % LINK_PROBE_HISTORY_SIZE];
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_LinkProbeHandler)

