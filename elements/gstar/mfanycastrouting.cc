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
/************************************************************************
 * MF_AnycastRouting.cc
 * This element implements anycast routing of datagrams that request it 
 ************************************************************************/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>
#include "mfanycastrouting.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

MF_AnycastRouting::MF_AnycastRouting():logger(MF_Logger::init()) {
}

MF_AnycastRouting::~MF_AnycastRouting() {
}

int MF_AnycastRouting::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse (conf, this, errh,
        "MY_GUID", cpkP+cpkM, cpUnsigned, &_myGUID,
        "FORWARDING_TABLE", cpkP+cpkM, cpElement, &_forwardElem,
        "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
         cpEnd) < 0) {
    return -1;
  }
  return 0;
}

// need to do next hop look-up based on the network address...
// set annotation at offset 0 to be the next hop GUID.
void MF_AnycastRouting::push(int port, Packet *p) {

  if (port != 0) {
    logger.log(MF_FATAL, 
      "acast_rtg: Packet received on unsupported port");
    exit(-1);
  }

  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)(p->data());
  uint32_t sid = chk_trans->sid;
  uint32_t chunk_id = chk_trans->chunk_id;
  Chunk *chunk = _chunkManager->get( chunk_id, ChunkStatus::ST_INITIALIZED );
  if (!chunk) {
    logger.log( MF_ERROR, "acast_rtg: cannot find chunk"); 
  }
  RoutingHeader *rheader = chunk->routingHeader();
  
  GUID dst_GUID = rheader->getDstGUID(); 
  NA dst_NA;
  dst_NA.init((uint32_t)0);
  uint32_t na_num = chunk->addressOptionsNum();

  logger.log(MF_DEBUG, 
    "acast_rtg: got chunk_id: %u dst_GUID: %u, num dst_NA: %u", 
    chunk_id, dst_GUID.to_int(), na_num);
  Vector<NA> *bound_NAs = chunk->addressOptions();

  /* if net addr has been bound, use network address */
  if (na_num == 0) {
    //no NA resolved; do GUID-based forwarding
    logger.log( MF_DEBUG,
      "acast_rtg: no NA resolved, do GUID-based forwarding"); 
  } else if (na_num == 1) {
    //NA already determined, forward on.
    rheader->setDstNA( bound_NAs->at(0) ); 
  } else {
    int min_pathlen = -1;
    uint32_t min_pathlen_NA = 0;
    /* Choose among 1 or more NAs */
    /* TODO: Current approach simply chooses NA least hops
     * away from this router. It doesn't consider 
     * latency, nor does it consider path quality 
     */
    for (int i = 0; i < na_num; i++) {
      int plen = _forwardElem->get_path_length(bound_NAs->at(i).to_int()); 
      if (plen >= 0) {
        if ((min_pathlen < 0) || (plen < min_pathlen)) {
          min_pathlen = plen;
          min_pathlen_NA = bound_NAs->at(i).to_int(); 
        }
      }
    }
    if (min_pathlen_NA) {
      logger.log(MF_DEBUG, 
        "acast_rtg: Min length NA for guid %u found: %u;", 
        dst_GUID.to_int(), min_pathlen_NA);
      /* dst NA decided, set in header packet */
      rheader->setDstNA(min_pathlen_NA); 
    }
  }
  output(0).push(p); 
}

CLICK_DECLS
EXPORT_ELEMENT(MF_AnycastRouting)
ELEMENT_REQUIRES(userlevel)
