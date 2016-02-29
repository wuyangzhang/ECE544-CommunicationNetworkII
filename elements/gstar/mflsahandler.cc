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
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "mflsahandler.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_LSAHandler::MF_LSAHandler(): _seq_no(0), timer(this), 
                                logger(MF_Logger::init()){
}

MF_LSAHandler::~MF_LSAHandler(){

}

int MF_LSAHandler::initialize(ErrorHandler *){
  timer.initialize(this);
  timer.schedule_after_msec(LSA_PERIOD_MSECS);
  return 0;
}

int MF_LSAHandler::configure(Vector<String> &conf, ErrorHandler *errh){
  if (cp_va_kparse(conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpInteger, &my_guid,
        "NEIGHBOR_TABLE", cpkP+cpkM, cpElement, &neighbor_tbl_p,
        "ROUTING_TABLE", cpkP+cpkM, cpElement, &routing_tbl_p,
        cpEnd) < 0) {
    return -1;
  }
  return 0; 
}

void MF_LSAHandler::run_timer(Timer *){

  Vector<const neighbor_t*> nbrs_vec;

  neighbor_tbl_p->get_neighbors(nbrs_vec);
  int length = nbrs_vec.size();
	
  if (length == 0) {
    logger.log(MF_DEBUG, "lsa_handler: No neighbors yet!");
    //reschedule task after lsa interval
    timer.schedule_after_msec(LSA_PERIOD_MSECS);	
    return;
  }

  //TODO define a struct for an lsa entry
  int packet_size = sizeof(LSA) + length * (sizeof(uint32_t)
				+ sizeof(uint32_t) + sizeof(uint32_t));
  int headroom = sizeof(click_ether) + sizeof(click_ip); 
	
  WritablePacket *packet = Packet::make(headroom, 0, packet_size, 0);

  if (!packet) {
    logger.log(MF_CRITICAL, "lsa_handler: can't make LSA packet!");
    return;
  }

  memset(packet->data(), 0, packet_size);
	
  LSA * header = (LSA *) packet->data();
       
  header->type = htonl(LSA_PKT); 
  header->senderLSA = htonl(my_guid); 
  header->sourceLSA = htonl(my_guid);
  //destination is broadcast
  header->destinationLSA = htonl(BROADCAST_GUID); 
  header->seq = htonl(_seq_no); 
  header->size = htonl(length); 

  //insert SETTs in LSA
  uint32_t * sett = (uint32_t *) (header + 1);
  for (int i=0; i < length; i++) {
    *sett = nbrs_vec[i]->s_ett;
    sett++;
  }
	
  //insert LETTs in LSA
  uint32_t * lett = sett;
  for (int i = 0; i < length; i++) {
    *lett = nbrs_vec[i]->l_ett;
    lett++;
  }
	
  //insert IDs in LSA
  uint32_t * address = lett;
  for (int i = 0; i < length; i++) {
    *address = nbrs_vec[i]->guid;
    address++;
  }
	
  _seq_no++;

  //sending packet
  logger.log(MF_DEBUG, 
    "lsa_handler: Sending new LSA with %u entries", ntohl(header->size));

  output(0).push(packet);

  //reschedule task after lsa interval
  timer.schedule_after_msec(LSA_PERIOD_MSECS);	
}

void MF_LSAHandler::push (int port, Packet * pkt){
  if (port == 0) {
    /* lsa message */
    handleLSA(pkt);
  } else {
    logger.log(MF_FATAL, 
      "lsa_handler: Packet received on unsupported port");
    exit(-1);
  }
}

void MF_LSAHandler::handleLSA(Packet *pkt){
  int lsa_flag;

  LSA * header = (LSA *)pkt->data();
	
  uint32_t src_guid = ntohl(header->sourceLSA); 
  uint32_t num_entries = ntohl(header->size); 

  /* skip any LSAs that originated at this node */
  if (src_guid == my_guid) {
    pkt->kill();
    return;
  }

  logger.log(MF_DEBUG, 
    "lsa_handler: Recvd LSA from node %u with %u entries", 
    src_guid, num_entries);
	//TODO pull data out and pass a well formed struct for insertion into RT
  lsa_flag = routing_tbl_p->insertLSA(pkt); 
	
  // if the LSA is new, rebroadcast it
  if (lsa_flag == 1) {	
    pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, BROADCAST_GUID);
    header->senderLSA = htonl(my_guid);
    logger.log(MF_DEBUG, 
      "lsa_handler: Broadcasting new LSA from node %u with %u entries", 
      src_guid, num_entries);

    output(0).push(pkt);	
  } else {
    pkt->kill();
  }
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_LSAHandler)
