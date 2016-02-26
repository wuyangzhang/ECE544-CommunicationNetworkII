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
#ifndef MF_CHUNK_HH
#define MF_CHUNK_HH

#include <click/packet.hh>
#include <click/hashtable.hh>
#include <click/timestamp.hh>
#include <click/vector.hh>
#include <click/timer.hh>

#include "mf.hh"
#include "mfchunkmeta.hh"
#include "mfcsyntimer.hh"
#include "mfchunkstatus.hh"

#define UNINITIALIZED_CHUNK_ID 0
CLICK_DECLS

class Chunk{
public:
  Chunk (uint32_t chunk_id, Vector<Packet*> *pv);
  Chunk (uint32_t chunk_id, uint32_t pkt_cnt); 
  Chunk (const Chunk &); 
  ~Chunk() {
    delete this->all_pkts;
    delete this->ext_headers;
    delete this->address_options;
    delete this->csyn_timer; 
  }
  
  inline RoutingHeader* routingHeader() {
    return this->routing_header; 
  }
  inline Vector<Packet*>* allPkts() const {
    return this->all_pkts;
  }
  inline char* ackBits() {
    return chunk_meta->ackBits();
  }
  inline Vector<ExtensionHeader*>* nextHeaders() {
    return this->ext_headers; 
  }
  inline Vector<NA> * addressOptions() const {
    return this->address_options;
  }
  inline MF_CSYNTimer * csynTimer() {
    return this->csyn_timer; 
  }

  inline MF_ServiceID getServiceID() {
    return this->routing_header->getServiceID(); 
  }
  inline uint32_t getUpperProtocol() {
    return this->routing_header->getUpperProtocol(); 
  }
  
  inline GUID& getSrcGUID() {
    return this->routing_header->getSrcGUID(); 
  }
  inline NA& getSrcNA() {
    return this->routing_header->getSrcNA(); 
  }
  inline GUID& getDstGUID() {
    return this->routing_header->getDstGUID(); 
  }
  inline NA& getDstNA() {
    return this->routing_header->getDstNA(); 
  }
   
  inline Timestamp getNABoundTime() {
    return this->ts_NA;
  }
  inline void setNABoundTime(Timestamp ts) {
    this->ts_NA = ts;
  }
 
  inline Timestamp getNextHopBoundTime() {
    return this->ts_nxthop;
  }
  inline void setNextHopBoundTime(Timestamp ts) {
    this->ts_nxthop = ts; 
  }

  uint32_t getChunkID() {
    return this->chunk_meta->getChunkID(); 
  }
  void setChunkID(uint32_t chunk_id) {
    chunk_meta->setChunkID(chunk_id); 
  }
  
  ChunkStatus getStatus() const {
    return this->chunk_status;
  }
  void setStatus(const ChunkStatus &chk_stat ) {
    this->chunk_status = chk_stat; 
  }
  void setStatus(uint16_t chk_stat ) {
    this->chunk_status = chk_stat; 
  }

  inline int32_t insert(uint32_t seq_num, Packet * pkt) {
    if (chunk_status.is(ChunkStatus::ST_COMPLETE)) {  //cannot insert or change a packet to a completed chunk
      return -1; 
    }
    if (seq_num < all_pkts->size()) { 
      all_pkts->at(seq_num) = pkt;
      chunk_meta->setAckBit(seq_num); 
      ++chk_pkt_cnt; 
      if (seq_num == 0) {
        this->init(pkt); 
        chunk_status.setInitialized();
      }
      if (chk_pkt_cnt == all_pkts->size()) {
        chunk_status.setComplete();
      }
      return chk_pkt_cnt; 
    } else {
      return -1; 
    }
  }

  uint32_t getChkPktCnt() const {
    return this->chk_pkt_cnt;
  }

  inline uint32_t setAddressOptions(NA_entry_t *NAs, uint32_t na_num) {
    int32_t bound_cnt = 0;
    address_options->clear(); 
    for( uint32_t i = 0; i != na_num; ++i ) {
      if( NAs[i].net_addr != 0) {
        NA *na = new NA();
        na->init(NAs[i].net_addr); 
        address_options->push_back(*na);
        ++bound_cnt; 
      }
    }
    return bound_cnt; 
  }
  
  inline void clearAddressOptions() {
    address_options->clear(); 
  }

  inline uint32_t addressOptionsNum() {
    return address_options->size(); 
  }
  
  inline void setNextHopGUID(const GUID &guid) {
    next_hop_GUID = guid; 
  }

  inline void setNextHopGUID(uint32_t guid) {
    next_hop_GUID = guid; 
  }

  inline GUID& getNextHopGUID() {
    return next_hop_GUID; 
  }
  
private:
  bool init(Packet * first_pkt);
  void initAddressOptions(); 
  RoutingHeader *routing_header;
  Vector<ExtensionHeader*> *ext_headers;
  Vector<Packet*> *all_pkts;
  Vector<NA> *address_options; 
  //char* ack_bits; 

  GUID next_hop_GUID;  
  Timestamp ts_assm;
  Timestamp ts_NA; 
  Timestamp ts_nxthop; 
  
  //uint32_t chunk_id;
  ChunkMeta *chunk_meta; 
  uint32_t chk_pkt_cnt;  
  ChunkStatus chunk_status;           //each bit of chunk_status is a flag
  MF_CSYNTimer *csyn_timer;
};

inline Chunk::Chunk(uint32_t chunk_id, uint32_t pkt_cnt) : chunk_status(), 
                                                           chk_pkt_cnt(0) {
  chunk_meta = new ChunkMeta(chunk_id, pkt_cnt); 
  this->all_pkts = new Vector<Packet *>(pkt_cnt, NULL);
  this->address_options = new Vector<NA>();
  this->csyn_timer = new MF_CSYNTimer(); 
}
bool inline Chunk::init(Packet *first_pkt) {
  this->next_hop_GUID.init(); 

  this->routing_header = new RoutingHeader(first_pkt->data() + HOP_DATA_PKT_SIZE);
  this->ext_headers = new Vector<ExtensionHeader*>();
  if (routing_header->hasExtensionHeader()) {
    ExtensionHeader *ext_hdr = new ExtensionHeader(routing_header->getHeader() + 
                                         routing_header->size());
    while (ext_hdr != NULL) {
      ext_headers->push_back(ext_hdr);
      ext_hdr = ext_hdr->getNextHeader();
    }
  }
  //process bound NAs
  this->ts_assm = Timestamp::now_steady();
  this->ts_NA = Timestamp::uninitialized_t();
  this->ts_NA.assign(0,0);
  this->ts_nxthop = Timestamp::uninitialized_t();
  this->ts_nxthop.assign(0,0);
  
  initAddressOptions(); 
}

void inline Chunk::initAddressOptions() {
  for (uint32_t i = 0; i != ext_headers->size(); ++i) {
    //if multihoming
    if (ext_headers->at(i)->getServiceID().isMultiHoming()) {
      MultiHomingExtHdr *mhome_hdr = new MultiHomingExtHdr(ext_headers->at(i)->getHeader());
      for (uint32_t j = 0; j != mhome_hdr->getNumOfDstNA(); ++j) {
        address_options->push_back(mhome_hdr->getDstNA(i)); 
      }
      if ((address_options->size() == 0) && (routing_header->getDstNA() != 0)) {
        address_options->push_back(routing_header->getDstNA()); 
      }
      //return; 
    }
  }
}

inline Chunk::Chunk(uint32_t chunk_id, Vector<Packet*> *vp) 
    : chk_pkt_cnt(vp->size()), chunk_status() {
  this->chunk_meta = new ChunkMeta(chunk_id, chk_pkt_cnt); 
  this->address_options = new Vector<NA>();
  this->all_pkts = vp;
  Packet *first_pkt = vp->at(0);
  init(first_pkt);

  this->csyn_timer = new MF_CSYNTimer(); 
  this->chunk_status.set(ChunkStatus::ST_INITIALIZED | ChunkStatus::ST_COMPLETE); 
}

inline Chunk::Chunk(const Chunk& chk) 
    : chk_pkt_cnt(chk.chk_pkt_cnt) {
  this->chunk_meta = new ChunkMeta(0, chk.chk_pkt_cnt); 
  this->chunk_status = chk.chunk_status;
  this->all_pkts = new Vector<Packet*>(chk_pkt_cnt, NULL);
  if (this->chunk_status.is(ChunkStatus::ST_COMPLETE)) {
    for (uint32_t i = 0; i != chk_pkt_cnt; ++i) {
      Packet* p = chk.allPkts()->at(i);
      assert(p);
      Packet* p_clone = p->clone();
      assert(p_clone); 
      this->all_pkts->at(0) = p_clone; 
    }
    this->address_options = new Vector<NA>(*(chk.addressOptions())); 
  } 
}


CLICK_ENDDECLS
#endif
