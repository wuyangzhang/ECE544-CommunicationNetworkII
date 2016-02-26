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

#include "mfbitratehandler.hh"

CLICK_DECLS

MF_BitrateHandler::MF_BitrateHandler()
    : local_lookup_id(0), logger(MF_Logger::init()) {
  lookupTable = new LocalLookupTable (); 
}

MF_BitrateHandler::~MF_BitrateHandler() {
};

int MF_BitrateHandler::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
                   "ARP_TABLE", cpkP+cpkM, cpElement, &arp_tbl_p,
                   "BITRATE_CACHE", cpkP+cpkM, cpElement, &_bitrateCache,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   cpEnd) < 0) {
    return -1; 
  }
  return 0; 
}

void MF_BitrateHandler::push(int port, Packet *p) {
  if (port == 0) {
    handleBitrateReqResp(p); 
  } else if (port == 1) {
    handleHostBitrate(p); 
  } else {
    logger.log(MF_ERROR, 
               "br_hdlr: Unrecognized packet on port %d!", port);
    p->kill(); 
  }
}

void MF_BitrateHandler::handleBitrateReqResp(Packet *p) {
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)p->data();
  uint32_t chunk_id = chk_trans->chunk_id;
  Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_COMPLETE); 
  if (!chunk) {
    logger.log(MF_ERROR,
               "br_hdlr: Cannot find chunk_id: %u in ChunkManager"); 
    p->kill(); 
  }
  RoutingHeader *rheader = chunk->routingHeader();
  uint32_t type = ntohl(*(uint32_t*)(rheader->getHeader() + 
                          rheader->getPayloadOffset()));

  if (type == BITRATE_REQ) {
    bitrate_req_t *req = (bitrate_req_t*)(rheader->getHeader() +
                                            rheader->getPayloadOffset());
    uint32_t dst_GUID = rheader->getSrcGUID().to_int();
    uint32_t client_GUID = ntohl(req->GUID);
    logger.log(MF_DEBUG,
               "br_hdlr: received a bitrate request for client GUID: %u", 
               client_GUID);
    lookupHostBitrate(arp_tbl_p->getMAC(client_GUID));
    lookupTable->set(local_lookup_id, chunk); 
    ++local_lookup_id;

  } else if (type == BITRATE_RESP) {
    bitrate_resp_t *resp = (bitrate_resp_t*)(rheader->getHeader() +
                                             rheader->getPayloadOffset());

    uint32_t client_GUID = ntohl(resp->GUID);
    uint32_t edge_router_GUID = ntohl(resp->edge_router_GUID);
    double bitrate = ntohl(resp->bitrate);
    
    _bitrateCache->insert(client_GUID, edge_router_GUID, bitrate);
    
    //TODO:remove chunk from chunk manager
    //_chunkManager->remove(chunk_id); 
  } else {
    logger.log(MF_ERROR, "br_hdlr: unsupported type"); 
  }
  p->kill(); 
}

bool MF_BitrateHandler::sendBitrateResp (uint32_t dst_GUID, uint32_t dst_NA,
             uint32_t req_GUID, double bitrate, uint32_t valid_sec){
  
  WritablePacket *pkt = Packet::make(sizeof(click_ether), 0, HOP_DATA_PKT_SIZE +
                          ROUTING_HEADER_SIZE + sizeof(bitrate_resp_t), 0);
  if (!pkt) {
    logger.log( MF_FATAL, "br_hdlr: cannot make packet!");
    return false;
  }
  memset(pkt->data(), 0, pkt->length());
  
  //fill hop data header
  hop_data * pheader = (hop_data*)(pkt->data());
  pheader->type = htonl(DATA_PKT);
  pheader->pld_size = htonl(ROUTING_HEADER_SIZE + sizeof(bitrate_resp_t));
  pheader->seq_num = htonl(0);                //only one pkt
  pheader->hop_ID = htonl(0);                 //not sure, should be reset before send out. 

  //fill routing header
  routing_header *rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE);
  rheader->setServiceID(MF_ServiceID::SID_UNICAST);
  rheader->setUpperProtocol(BITRATE);        //2 means bitrate reply pkt
  rheader->setSrcGUID(_myGUID);
  rheader->setDstGUID(dst_GUID);
  rheader->setDstNA(dst_NA);
  rheader->setPayloadOffset(ROUTING_HEADER_SIZE);
  rheader->setChunkPayloadSize(sizeof(bitrate_req_t));

  //fill bitrate_resp 
  bitrate_resp_t *resp = (bitrate_resp_t*)(rheader->getHeader() +
                            rheader->getPayloadOffset());
  resp->type = htonl(BITRATE_RESP); 
  resp->GUID = htonl(req_GUID);
  resp->edge_router_GUID = htonl(_myGUID);
  resp->bitrate = htonl(bitrate);
  resp->valid_time_sec = htonl(valid_sec);

  WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
  if (!chk_pkt) {
    logger.log(MF_FATAL, "br_hdlr: cannot make packet!");
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

  logger.log(MF_DEBUG,
             "br_hdlr: Sent bitrate resp to dst guid: %u, dst NA: %u, "
             "client GUID: %u bitrate: %u valid_sec: %u",
             dst_GUID, dst_NA, req_GUID, bitrate, valid_sec);
  output(0).push(chk_pkt);
  return true;
}

void MF_BitrateHandler::handleHostBitrate (Packet *p) {
  bitrate_local_resp_t *resp = (bitrate_local_resp_t*)p->data();
  uint32_t lookup_id = ntohl(resp->lookup_id);
  uint32_t bitrate = ntohl(resp->bitrate);
  uint32_t valid_sec = ntohl(resp->valid_sec); 
  logger.log(MF_DEBUG, 
             "br_hdlr: Got local lookup resp, lookup_id: %u, bitrate: %u, "
             "valid_sec: %u",
             lookup_id, bitrate, valid_sec);
  LocalLookupTable::iterator it = lookupTable->find(lookup_id);
  if (it == lookupTable->end()) {
    logger.log(MF_ERROR,
               "br_hdlr: lookup_id: %u is missing!", lookup_id);
    return; 
  } else {
    Chunk *chunk = it.value(); 
    if (!chunk) {
      logger.log(MF_ERROR,
                 "br_hdlr: Got an invalid chunk from local lookup table");
      return; 
    }

    uint32_t chunk_id = chunk->getChunkID(); 
    RoutingHeader *rheader = chunk->routingHeader();
    bitrate_req_t *req = (bitrate_req_t*)(rheader->getHeader() +
                                          rheader->getPayloadOffset());

    uint32_t type = ntohl(req->type); 
    if (type != BITRATE_REQ) {
      logger.log(MF_ERROR, 
                 "br_hdlr: Record type doesn't match"); 
    }

    uint32_t dst_GUID = rheader->getSrcGUID().to_int();
    uint32_t client_GUID = ntohl(req->GUID);
    sendBitrateResp(dst_GUID, dst_GUID, client_GUID, bitrate, valid_sec);
    //_chunkManager->remove(chunk_id);
    _chunkManager->removeData(chunk); 
  }
}

void  MF_BitrateHandler::lookupHostBitrate (EtherAddress ether_addr) {
  String mac = ether_addr.unparse_colon().lower(); 
  
  if (mac ==  "00:00:00:00:00:00") {
    logger.log(MF_DEBUG, "br_hdlr: no such client"); 
  }

  logger.log(MF_DEBUG,
             "br_hdlr: look up bitrate of local client with mac address %s", mac.c_str());
  
  uint32_t msg_size = sizeof(bitrate_local_lookup_t);
  WritablePacket *bitrate_lookup = Packet::make(0, 0, sizeof(bitrate_local_lookup_t), 0);
  if (!bitrate_lookup) {
    logger.log(MF_FATAL, "br_hdlr: cannot make packet!");
    return; 
  }
  
  memset(bitrate_lookup->data(), 0, bitrate_lookup->length());
  bitrate_local_lookup_t *local_lookup = (bitrate_local_lookup_t*)bitrate_lookup->data();
  local_lookup->lookup_id = htonl(local_lookup_id);
  memcpy(local_lookup->mac, ether_addr.data(), 6);  
    
  output(1).push(bitrate_lookup); 
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_BitrateHandler)
ELEMENT_REQUIRES(userlevel)
