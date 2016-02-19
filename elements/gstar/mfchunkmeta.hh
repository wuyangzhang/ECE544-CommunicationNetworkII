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
#ifndef MF_CHUNK_META_HH
#define MF_CHUNK_META_HH
CLICK_DECLS

class ChunkMeta {
 public:
  ChunkMeta(uint32_t cid, uint32_t pkt_num) : chunk_id(cid), 
                                              chunk_pkt_num(pkt_num) {
    this->ack_bits = new char[chunk_pkt_num/8 + 1]();
  }

  ~ChunkMeta() {
    delete ack_bits; 
  }
  inline uint32_t getChunkID() {
    return chunk_id; 
  }
  inline void setChunkID(uint32_t cid) {
    chunk_id = cid; 
  }
  inline char *ackBits() {
    return ack_bits; 
  } 
  inline bool setAckBit(uint32_t seq_num) {
    if (seq_num >= chunk_pkt_num) {
      return false; 
    }
    ack_bits[(seq_num) / 8] |= 0x01 << ((seq_num) % 8);   //set_ack_bits;
    return true; 
  }
 private:
  uint32_t chunk_id;
  uint32_t chunk_pkt_num; 
  char *ack_bits; 
}; 

CLICK_ENDDECLS

#endif
