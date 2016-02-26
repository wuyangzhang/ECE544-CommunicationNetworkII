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
/*****************************************************************
 * MF_ChunkSource.cc
 * 
 * Creates and outputs chunks with specified properties. Properties 
 * include:
 * chunk size 
 * SID (service ID)
 * source GUID
 * dest GUID
 * inter-chunk interval
 * number of chunks, -1 for infinite
 * L2 pkt size for parameter eval
 * delay - in starting transmission
 *
 * The data block is constructed in the format expected by other elements
 * in the router pipeline, viz. a chain of MTU sized packets, with the 
 * L3, L4... headers present in the first packet. 
 *
 * Checks if downstream has capacity before pushing out a chunk, and
 * listens to downstream signals of available capacity to schedule the
 * next chunk out
 *
 *****************************************************************/

#include <click/config.h>
#include <click/args.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/vector.hh>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "mfchunksource.hh"


CLICK_DECLS

MF_ChunkSource::MF_ChunkSource() 
    : _task(this), _timer(&_task), _active(true), _start_msg_sent(false), 
     _curr_chk_ID(1), _curr_chk_cnt(0), _service_id(MF_ServiceID::SID_UNICAST),
     _pkt_size(DEFAULT_PKT_SIZE), _chk_intval_msec(0), _dst_NAs_str(), 
     _delay_sec(10), logger(MF_Logger::init()) {
  _dst_NAs = new Vector<NA>();
}

MF_ChunkSource::~MF_ChunkSource() {

}

int MF_ChunkSource::configure(Vector<String> &conf, ErrorHandler *errh) {

  if (cp_va_kparse(conf, this, errh,
                   "SRC_GUID", cpkP+cpkM, cpUnsigned, &_src_GUID,
                   "DST_GUID", cpkP+cpkM, cpUnsigned, &_dst_GUID,
                   "SIZE", cpkP+cpkM, cpUnsigned, &_chk_size,
                   "LIMIT", cpkP+cpkM, cpInteger, &_chk_lmt,
                   "ROUTER_STATS", cpkP+cpkM, cpElement, &_routerStats,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                   "SERVICE_ID", cpkP, cpUnsigned, &_service_id,
                   "PKT_SIZE", cpkP, cpUnsigned, &_pkt_size,
                   "INTERVAL", cpkP, cpUnsigned, &_chk_intval_msec,
                   "DST_NA", cpkP, cpString, &_dst_NAs_str,
                   "DELAY", cpkP, cpUnsigned, &_delay_sec,
                   cpEnd) < 0) {
    return -1;
  }
  return 0; 
}

int MF_ChunkSource::initialize(ErrorHandler * errh) {
  //check dst na config
  if (parseDstNAs()) {
    logger.log(MF_DEBUG, "chnk_src: Parse all Dst_NAs successfully"); 
  } else {
    logger.log(MF_ERROR, "chnk_src: configuration of Dst_NAs is wrong"); 
  }
  
  //check pkt size and chk size config
  if (_pkt_size > _chk_size) {
    logger.log(MF_ERROR, 
               "chnk_src: configuration error!"); 
  }
  logger.log(MF_INFO, 
             "chnk_src: Sending chunks of size:  %u, MTU: %u, count %u, "
             "interval: %u msecs", 
             _chk_size, _pkt_size, _chk_lmt, _chk_intval_msec);
  logger.log(MF_INFO, "chnk_src: service ID: %x", _service_id); 
        
  //init task, but use timer to schedule after delay
  ScheduleInfo::initialize_task(this, &_task, false, errh);
  _timer.initialize(this);
  /* listen for downstream capacity available notifications */
  _signal = Notifier::downstream_full_signal(this, 0, &_task);

  _timer.schedule_after_msec(_delay_sec * 1000);
  logger.log(MF_INFO, 
             "chnk_src: Transmission begins in: %u sec", _delay_sec);
  return 0;
}

bool MF_ChunkSource::run_task(Task *) {
  
  if (!_active) {
    return false;    //don't reschedule 
  }
  
  //check if downstream is full
  if (!_signal) {
    return false; //don't reschedule wait for notification
  }
  
  uint32_t new_intval = 0; 
  if (_routerStats->isBusy()) {
    if (_chk_intval_msec == 0) {
      new_intval = 10; 
    } else {
      new_intval = 2 * _chk_intval_msec; 
    }
    logger.log(MF_DEBUG,
      "chnk_src: Busy!! inter-chunk delay, cheduling after %u msecs",
      new_intval);
    _timer.reschedule_after_msec(new_intval);
    return false;
  }

  //send start timer msg to receiver for measuring bitrate
  if (!_start_msg_sent) {
    sendStartMeasurementMsg();
    _start_msg_sent = true;
    _task.reschedule(); 
    return true;
  }
    
  uint32_t pkt_cnt = 0;
  uint32_t curr_chk_size = 0;
  char *payload;
  routing_header *rheader = NULL;
  transport_header *theader = NULL;
  Chunk *chunk = NULL;
  Vector<Packet*> *chk_pkts = 0; 
  bool empty_pkt;

  // prepare a convenience Chunk pkt for Click pipeline
  WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0); 
  if (!chk_pkt) {
    logger.log(MF_FATAL, "chnk_src: can't make chunk packet!");
    exit(EXIT_FAILURE);
  }
  memset(chk_pkt->data(), 0, chk_pkt->length());

  WritablePacket *pkt = NULL;   
  while (curr_chk_size < _chk_size) {
    
    if (pkt_cnt == 0) {   //if first pkt
      pkt = Packet::make(sizeof(click_ether), 0, _pkt_size, 0); 
      if (!pkt) {
        logger.log(MF_FATAL, "chnk_src: can't make packet!");
        exit(EXIT_FAILURE);
      }
      memset(pkt->data(), 0, pkt->length());
    
      /* HOP header */
      hop_data_t * pheader = (hop_data_t *)(pkt->data());
      pheader->type = htonl(DATA_PKT);
      pheader->seq_num = htonl(pkt_cnt);                    //starts at 0
      pheader->hop_ID = htonl(_curr_chk_ID);

      //first pkt of chunk -- add L3, L4 headers, followed by data
      //L3  header
      rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE);
      rheader->setVersion(1);
      switch (_service_id) {
       case MF_ServiceID::SID_UNICAST:
        break;
       case MF_ServiceID::SID_ANYCAST:
        break;
       case MF_ServiceID::SID_MULTICAST:
        break; 
       case MF_ServiceID::SID_MULTIHOMING:
        rheader->setServiceID(MF_ServiceID::SID_MULTIHOMING | 
                              MF_ServiceID::SID_EXTHEADER);
        logger.log(MF_DEBUG, 
                   "chnk_src: set SID_MULTIHOMING and SID_EXTHEADER"); 
        break;
       default:
        logger.log(MF_ERROR, 
                   "chnk_src: unsupported Service ID, set to Unicast");
        rheader->setServiceID(MF_ServiceID::SID_UNICAST); 
        break; 
      }
    
      //use transport protocal
      rheader->setUpperProtocol(TRANSPORT); 
      rheader->setDstGUID(_dst_GUID);
      //if dst NA is defined in config file
      if (_dst_NAs->size() != 0) {
        //use the first one
        rheader->setDstNA(_dst_NAs->at(0));
        logger.log(MF_DEBUG, 
                   "chnk_src: set routing header dst NA: %u", 
                   _dst_NAs->at(0).to_int()); 
      }
      rheader->setSrcGUID(_src_GUID);
      rheader->setSrcNA(0);
      
      uint32_t extensionHeaderSize = 0;
      if (rheader->hasExtensionHeader()) {
        if (rheader->getServiceID().isMultiHoming()) {
          MultiHomingExtHdr *extHeader;
          //if dst NAs are defined in the config file
          if (_dst_NAs->size() != 0) {
            //use defined NAs 
            extHeader = new MultiHomingExtHdr(pkt->data() + 
                HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE, 0, _dst_NAs->size());
            for (uint32_t i = 0; i != _dst_NAs->size(); ++i) {
              extHeader->setDstNA(_dst_NAs->at(i), i); 
            }
            extensionHeaderSize = extHeader->size();
            char buf[512];
            logger.log(MF_DEBUG,
                       "chnk_src: mutlihoming next header info: %s",
                       extHeader->to_log(buf));
          } else {  //if no dst NAs are defined in config file
            //reserve EXTHDR_MULTIHOME_MAX_SIZE
            extensionHeaderSize = EXTHDR_MULTIHOME_MAX_SIZE;
          } 
        } else {
          logger.log(MF_DEBUG, "chnk_src: no implementation"); 
        }
      }
      uint32_t payloadOffset = ROUTING_HEADER_SIZE + extensionHeaderSize; 
      rheader->setPayloadOffset(payloadOffset);
      char buf[512];
      logger.log(MF_DEBUG,
                 "routing header info: %s", rheader->to_log(buf));
      chk_pkts = new Vector<Packet*> ();              
      //L4  header
      theader = (transport_header*) (pkt->data() + HOP_DATA_PKT_SIZE + 
                    payloadOffset);
      /*
      theader->chk_ID = htonl(_curr_chk_ID);
      theader->chk_size = htonl(_chk_size);	
      theader->src_app_ID = 0;
      theader->dst_app_ID = 0;
      theader->reliability = 0;
      */
      theader->chunkID = htonl(_curr_chk_ID);
      theader->chunkSize = htonl(_chk_size);
      theader->srcTID = 0;
      theader->dstTID = 0;
      theader->msgID = 0;
      theader->msgNum = 0;
      theader->offset = 0; 
      
      curr_chk_size += _pkt_size - (sizeof(click_ether) + payloadOffset);
      pheader->pld_size = htonl(_pkt_size - (sizeof(click_ether) 
                + HOP_DATA_PKT_SIZE));
    } else {
      uint32_t remaining_bytes = _chk_size - curr_chk_size;
      uint32_t bytes_to_send = _pkt_size;
      // if remaining data size is less than data pkt payload
      if (remaining_bytes < (_pkt_size - sizeof(click_ether) - 
              HOP_DATA_PKT_SIZE)) {
         bytes_to_send = remaining_bytes + sizeof(click_ether) + 
              HOP_DATA_PKT_SIZE;
      }

      uint32_t pkt_payload = bytes_to_send - sizeof(click_ether) -
                             HOP_DATA_PKT_SIZE;
      pkt = Packet::make(sizeof(click_ether), 0, bytes_to_send, 0);
      if (!pkt) {
          logger.log(MF_FATAL, "chnk_src: can't make packet!");
          exit(EXIT_FAILURE);
      }
      memset(pkt->data(), 0, pkt->length());
      
      /* HOP header */
      hop_data_t * pheader = (hop_data_t *)(pkt->data());
      pheader->type = htonl(DATA_PKT);
      pheader->seq_num = htonl(pkt_cnt);                    //starts at 0
      pheader->hop_ID = htonl(_curr_chk_ID);
      pheader->pld_size = htonl(pkt_payload);
      curr_chk_size += pkt_payload;  //curr_chk_size == chk_size, loop ends
    }
    chk_pkts->push_back(pkt); 
    ++pkt_cnt;
    logger.log(MF_TRACE, 
      "chnk_src: Created pkt #: %u for chunk #: %u, curr size: %u", 
       pkt_cnt, _curr_chk_ID, curr_chk_size);
    pkt = NULL; 
  }
  chunk = _chunkManager->alloc(chk_pkts, 
                     ChunkStatus::ST_INITIALIZED||ChunkStatus::ST_COMPLETE); 
  chk_internal_trans_t *chk_trans = ( chk_internal_trans_t*) chk_pkt->data(); 
  chk_trans->sid = htonl(chunk->getServiceID().to_int());
  chk_trans->chunk_id = chunk->getChunkID(); 
    
  output(0).push(chk_pkt);
  _curr_chk_cnt++;
  logger.log(MF_INFO, 
    "chnk_src: PUSHED ready chunk #: %u, chunk_id: %u size: %u, num_pkts: %u", 
    _curr_chk_ID, chunk->getChunkID(), _chk_size, pkt_cnt);
  _curr_chk_ID++;

  if (_chk_lmt > 0 && _curr_chk_cnt >= _chk_lmt) {
    _active = false; //done, no more
    return false;
  }

  /* if valid interval was specified, schedule after duration */
  if (_chk_intval_msec > 0) {
    logger.log(MF_DEBUG, 
      "chnk_src: inter-chunk delay, cheduling after %u msecs", 
      _chk_intval_msec);
    _timer.reschedule_after_msec(_chk_intval_msec);
    return false;
  }

  _task.fast_reschedule();
  return true;
}

bool MF_ChunkSource::sendStartMeasurementMsg() {
  uint32_t pkt_size = HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE + 
                      sizeof(start_measurement_msg_t); 
  WritablePacket *start_pkt = Packet::make(sizeof(click_ether), 0, pkt_size, 0);
  if (!start_pkt) {
    logger.log(MF_FATAL, "chnk_src: can't make packet!");
    exit(EXIT_FAILURE);
  }
  memset(start_pkt->data(), 0, start_pkt->length());
  
  uint32_t curr_chk_size = 0;

  //HOP header
  hop_data_t * pheader = (hop_data_t *)(start_pkt->data());
  pheader->type = htonl(DATA_PKT);
  pheader->seq_num = htonl(0);                //starts at 0
  pheader->hop_ID = htonl(_curr_chk_ID);      

  //routing header
  routing_header *rheader = 
      new RoutingHeader(start_pkt->data() + HOP_DATA_PKT_SIZE);
  rheader->setVersion(1);
  rheader->setServiceID(MF_ServiceID::SID_UNICAST);         //0-unicast
  rheader->setUpperProtocol(MEASUREMENT);                   //measurement pkt
  rheader->setDstGUID(_dst_GUID);
  rheader->setDstNA(0);
  rheader->setSrcGUID(_src_GUID);
  rheader->setSrcNA(0);
  rheader->setPayloadOffset(ROUTING_HEADER_SIZE);
  rheader->setChunkPayloadSize(sizeof(start_measurement_msg_t)); 

  start_measurement_msg_t *msg = 
      (start_measurement_msg_t*)(rheader->getHeader() + 
      rheader->getPayloadOffset());
  msg->num_to_measure = htonl(_chk_lmt); 
  
  pheader->pld_size = htonl(pkt_size);

  Chunk *chunk = _chunkManager->alloc(start_pkt, 1, ChunkStatus::ST_COMPLETE); 
  
  WritablePacket *chk_pkt = Packet::make(sizeof(click_ether), 0, 
                                         sizeof(chk_internal_trans_t), 0);
  memset(chk_pkt->data(), 0, chk_pkt->length()); 
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)chk_pkt->data();  
  chk_trans->sid = htonl(rheader->getServiceID().to_int());
  chk_trans->chunk_id = chunk->getChunkID();
  output(0).push(chk_pkt); 
  logger.log(MF_DEBUG,
    "chnk_src: PUSHED start measurement msg, chunk_id %u",
    chunk->getChunkID());
  _curr_chk_ID++; 
  return true; 
}

bool MF_ChunkSource::parseDstNAs() {
  //TODO+ tmp size should equal to MAX_NA_SIZE
  char tmp[256];
  uint32_t cnt = 0; 
  for (String::iterator it = _dst_NAs_str.begin(); it != _dst_NAs_str.end(); ++it) {
    if (*it >= '0' && *it <= '9') {
      tmp[cnt] = *it;
      ++cnt; 
    } else if ( *it == ':') {
      tmp[cnt] = '\0';
      NA na;
      na.init(atoi(tmp));
      _dst_NAs->push_back(na); 
      cnt = 0; 
    } else {
      return false; 
    }
  }
  logger.log(MF_DEBUG, 
             "chnk_src: dst NA: %s, %u NAs parsed", 
             _dst_NAs_str.c_str(), _dst_NAs->size()); 
  for (uint32_t i = 0; i != _dst_NAs->size(); ++i) {
    logger.log(MF_INFO, "chnk_src: NA#%u: %u", i, _dst_NAs->at(i).to_int() ); 
  }
  return true; 
}

void MF_ChunkSource::add_handlers() {

    add_data_handlers("count", Handler::OP_READ, &_curr_chk_cnt);
    add_task_handlers(&_task, &_signal);

}

/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<pkt*>;
template class Vector<int>;
#endif
*/
CLICK_ENDDECLS
EXPORT_ELEMENT(MF_ChunkSource)
ELEMENT_REQUIRES(userlevel)
