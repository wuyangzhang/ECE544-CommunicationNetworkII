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
#include <click/vector.hh>
#include <click/timestamp.hh>

#include <stdlib.h>

#include "mfmultihomerouting.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_MultiHomeRouting::MF_MultiHomeRouting():logger(MF_Logger::init()){
  _assignTable = new AssignTable;
}

MF_MultiHomeRouting::~MF_MultiHomeRouting(){
}

int MF_MultiHomeRouting::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
        "FORWARDING_TABLE", cpkP+cpkM, cpElement, &_forwardElem,
        "BITRATE_CACHE", cpkP+cpkM, cpElement, &_bitrateCache,
        "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager, 
        cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void MF_MultiHomeRouting::push(int port, Packet *p) {
  if (!port) {
    handleChunkPkt(p); 
  } else {
    logger.log(MF_FATAL,
      "mhome_rtg: Unrecognized packet on port %d!", port);
    p->kill(); 
  }
}

void MF_MultiHomeRouting::handleChunkPkt(Packet *p) {
  /* general algorithm here: if bifurcation node, split; else, forward all on */
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)p->data();
  uint32_t chunk_id = chk_trans->chunk_id;
  Chunk * chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
  if (!chunk) {
    logger.log(MF_ERROR, 
      "mhome_rtg: Cannot find chunk_id: %u in ChunkManaer");
    p->kill(); 
  }

  routing_header *rheader = chunk->routingHeader(); 
  uint32_t dst_GUID = rheader->getDstGUID().to_int(); 
  uint32_t dst_NA = rheader->getDstNA().to_int();


  logger.log(MF_DEBUG, 
    "mhome_rtg: Recvd chunk chunk_id: %u dstGUID: %u dstNA: %u", 
    chunk_id, dst_GUID, dst_NA);
  
  if (dst_NA == _myGUID) {
    output(0).push(p);
    return; 
  }

  NA assigned_NA;
  assigned_NA.init((uint32_t)0);
  uint32_t na_num = chunk->addressOptions()->size(); 
  
  logger.log(MF_DEBUG,
    "mhome_rtg: Chunk chunk_id: %u dstGUID: %u resolved to %u NAs",
     chunk_id, dst_GUID, na_num);

  if (na_num == 0) {
    logger.log(MF_DEBUG, 
      "mhome_rtg: chunk_id: %u, no bound NAs, GUID-based routing", chunk_id);
    output(0).push(p);
    return;       
    //assigned_NA = rheader->getDstNA().to_int();          //assigned_NA may be 0, in that case GUID-based routing                
  } 

  if (na_num == 1) {
    assigned_NA = chunk->addressOptions()->at(0);    //if only 1 na, assigned_NA
    rheader->setDstNA(assigned_NA);
    logger.log(MF_DEBUG, 
               "mhome_rtg: Chunk_id: %u, only 1 NA option, assigned NA: %u",
               chunk_id, assigned_NA.to_int());               
    output(0).push(p);
    return; 
  } 
  
  if (na_num > 1 && !chunk->getServiceID().isExtensionHeader()) { //if na space for next header
    uint32_t min_mhome_header_size = EXTHDR_MULTIHOME_SRC_NA_OFFSET + 
        na_num * ADDRESS_LENGTH;
    if (min_mhome_header_size > rheader->getPayloadOffset()) {
      //TODO: also we can split the packet into 2 packets
      assigned_NA = chunk->addressOptions()->at(0);    //use the first one
      logger.log(MF_DEBUG, 
                 "mhome_rtg: Chunk_id: %u, not enough reserved space for mhome" 
                 "header, use the first one, assigned NA: %u",
                 chunk_id, assigned_NA.to_int());
      rheader->setDstNA(assigned_NA); 
      output(0).push(p);
      return; 
    } else {
      rheader->setServiceID(MF_ServiceID::SID_EXTHEADER);
      MultiHomingExtHdr *mhome_hdr = new MultiHomingExtHdr(
          rheader->getHeader() + ROUTING_HEADER_SIZE, 0, na_num);
      for (uint32_t i = 0; i != na_num; ++i) {
        mhome_hdr->setDstNA(chunk->addressOptions()->at(i), i); 
      }
      logger.log(MF_DEBUG,
                 "mhome_rtg: Enough reserved space, fill mhome header"); 
    }
  } 
                            
  //na_num > 1, has next header                        
  if (na_num > 1 && chunk->getServiceID().isExtensionHeader()) {
    // if next hop for different NAs agrees
    ExtensionHeader *ext = rheader->getExtensionHeader(MF_ServiceID::SID_MULTIHOMING);         //
    if (ext == NULL) {
      logger.log(MF_DEBUG, 
                 "mhome_rtg: routing header has next header but not multihoming");
      output(0).push(p);
      return;
    }
  } 
      
  uint32_t next_hop[MAX_NUM_NA];
  
  for (uint32_t i = 0; i != na_num; ++i) {
    logger.log(MF_DEBUG, 
               "mhome_rtg: NA option #%u: %u",  
               i, chunk->addressOptions()->at(i).to_int()); 
    next_hop[i] = _forwardElem->getNextHopGUID(chunk->addressOptions()->at(i).to_int());
  }  
  // TODO: for simplicity, currently only handle two diff. NAs
  if (next_hop[0] == next_hop[1]) {              //if has the same next hop 
    assigned_NA = chunk->addressOptions()->at(0); 
    logger.log(MF_DEBUG,
               "mhome_rtg: same next hop: %u", next_hop[0]); 
  } else {                                         //if next hops are different
    logger.log(MF_DEBUG, 
               "mhome_rtg: different next hop: %u, %u", 
               next_hop[0], next_hop[1]);

    // if next hops same, bifurcation can be effected further downstream
    uint32_t client_bitrate[MAX_NUM_NA];
    for (int i = 0; i != MAX_NUM_NA; ++i) {
      client_bitrate[i] = 0;
    }

    //lookup link bandwidth in bitrate cache
    for (int i = 0; i != na_num; ++i) {
      double mbps = _bitrateCache->getBitrate(dst_GUID, 
                                              chunk->addressOptions()->at(i).to_int());
      if (mbps == 0) {
        sendBitrateReq(dst_GUID, chunk->addressOptions()->at(i).to_int(), next_hop[i]);
      } else {
        client_bitrate[i] = mbps;
      }
    }

    // else, use weights specified within GNRS mappings
    // or other path metrics to determine route
    if ((client_bitrate[0] != 0) && (client_bitrate[1] == 0)) {
       assigned_NA = chunk->addressOptions()->at(0);
    } else if ( (client_bitrate[0] == 0) && (client_bitrate[1] != 0) ){
       assigned_NA = chunk->addressOptions()->at(1); 
    } else if ( (client_bitrate[0] != 0) && (client_bitrate[1] == 0) ){
       assigned_NA = chunk->addressOptions()->at(0);  
    } else {                                       //if both of them have no record
      uint16_t wts[MAX_NUM_NA];                    //policy: split 
      assign_hist *hist = _assignTable->get(dst_GUID);
      if (hist == NULL) { 
        hist = new assign_hist();
        _assignTable->find_insert(dst_GUID, hist);
      }
         
      //TODO populate weights into packet meta data 
      // for each NA returned by GNRS - at NetBinder
      memset(wts, 0, sizeof(uint16_t) * MAX_NUM_NA);
      wts[0] = client_bitrate[0]; 
      wts[1] = client_bitrate[1];
      logger.log(MF_DEBUG, 
                 "mhome: weight for NA1: %u, NA2: %u", wts[0], wts[1]);
      /**
       * If any weight is 0, it is excluded. If all
       * weights are 0, then choice is made on another
       * metric - e.g., path length
       */
      // do NA allocation based on weight
      assigned_NA = assign_weighted_NA(chunk->addressOptions(), hist, wts); 
      if (!assigned_NA.to_int()) {
         //all weights zero
         //use path length metric to forward
         assigned_NA = assign_shortest_path_NA(chunk->addressOptions(), hist);
      }
      //TODO disable extheader is a temp solution, because there may be other extension headers
      rheader->resetServiceID(MF_ServiceID::SID_EXTHEADER); 
    }
  }  
  logger.log(MF_DEBUG, 
    "mhome_rtg: Chunk_id: %u, assigned NA: %u", chunk_id, assigned_NA.to_int());
  rheader->setDstNA(assigned_NA.to_int());
  output(0).push(p); 
}

bool MF_MultiHomeRouting::sendBitrateReq (uint32_t dst_GUID,
                            uint32_t dst_NA, uint32_t next_hop_GUID){
  WritablePacket *pkt = Packet::make(sizeof(click_ether), 0, HOP_DATA_PKT_SIZE + 
                          ROUTING_HEADER_SIZE + sizeof(bitrate_req_t), 0);
  if (!pkt) {
    logger.log( MF_FATAL, "mhome_rtg: cannot make packet!");
    return false;
  }
  memset(pkt->data(), 0, pkt->length());
  
  //fill Hop Header
  hop_data_t * pheader = (hop_data_t*)(pkt->data());
  pheader->type = htonl(DATA_PKT);
  pheader->pld_size = htonl(ROUTING_HEADER_SIZE + sizeof(bitrate_req_t));
  pheader->seq_num = htonl(0);                           //only one pkt
  pheader->hop_ID = htonl(0);                            //not sure, should be reset before send out. 
  
  //fill routing Header
  routing_header *rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE); 
  rheader->setServiceID(MF_ServiceID::SID_UNICAST); 
  rheader->setUpperProtocol(BITRATE);                   //1 means request packet;
  rheader->setSrcGUID(_myGUID);
  rheader->setDstGUID(dst_GUID);
  rheader->setDstNA(dst_NA);
  rheader->setPayloadOffset(ROUTING_HEADER_SIZE);
  rheader->setChunkPayloadSize(sizeof(bitrate_req_t)); 
  
  //fill bitreat_req
  bitrate_req_t *req = (bitrate_req_t*)(rheader->getHeader() + 
                                          rheader->getPayloadOffset()); 
  req->type = htonl(BITRATE_REQ); 
  req->GUID = htonl(dst_GUID);

  WritablePacket *chk_pkt = Packet::make(0, 0 ,sizeof(chk_internal_trans_t), 0);
  if (chk_pkt == 0) {
    logger.log( MF_FATAL, "mhome_rtg: cannot make packet!");
    pkt->kill(); 
    return false; 
  }
  memset(chk_pkt->data(), 0, chk_pkt->length());
  
  Chunk *chunk = _chunkManager->alloc(pkt, 1, 0);   //alloc a chunk with only 1 packet
  //fill internal trans msg
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)chk_pkt->data();
  chk_trans->sid = htonl(chunk->getServiceID().to_int());
  chk_trans->chunk_id = chunk->getChunkID();  
  chk_pkt->set_anno_u32(SRC_GUID_ANNO_OFFSET, _myGUID);
  chk_pkt->set_anno_u32(NXTHOP_ANNO_OFFSET, next_hop_GUID);

  logger.log(MF_DEBUG,
             "mhome_rtg: sent bitrate_req dst_GUID: %u, dstNA: %u",
             dst_GUID, dst_NA);
  _chunkManager->push_to_port(chk_pkt, PORT_TO_SEG);
  return true; 
}


uint32_t MF_MultiHomeRouting::assign_weighted_NA(Vector<NA> *vec_na, 
                                             struct assign_hist *hist, 
                                             uint16_t (&wts)[MAX_NUM_NA]) {

  uint32_t tot_wt = 0;
    
  for (int i = 0; i < MAX_NUM_NA; i++) {
    tot_wt += wts[i];
  }
  if (tot_wt == 0) {
    return vec_na->at(0).to_int(); 
   //return NAs[0]; 
  }
    //assumes tot_wt <= RAND_MAX
    //the '+ 1' below is to avoid first entry being chosen
    //when all wts are 0s
  int wt_ind = (random() % tot_wt) + 1;

  tot_wt = 0;
  for (int i = 0; i < MAX_NUM_NA; i++) {
    tot_wt += wts[i];
    if (tot_wt >= wt_ind) {
      hist->num_assigned++;
      hist->last_NA = vec_na->at(i).to_int();
      return vec_na->at(i).to_int();
    }
  }
  return 0; //shouldn't happen unless all wts are 0
}

uint32_t MF_MultiHomeRouting::assign_shortest_path_NA(Vector<NA> *vec_na, 
                                                  struct assign_hist *hist) {
  int min_plen = MAX_PATH_LENGTH;
  uint32_t min_plen_NA = 0;

  for (int i = 0; i < MAX_NUM_NA; i++) {
    int plen = _forwardElem->get_path_length(vec_na->at(i).to_int());
    if (plen && plen < min_plen) {
      min_plen_NA = vec_na->at(i).to_int();
    }
  }
  if (min_plen_NA) {
    hist->num_assigned++;
    hist->last_NA = min_plen_NA;
  }
  return min_plen_NA;
}
CLICK_ENDDECLS
EXPORT_ELEMENT(MF_MultiHomeRouting)
ELEMENT_REQUIRES(userlevel)
