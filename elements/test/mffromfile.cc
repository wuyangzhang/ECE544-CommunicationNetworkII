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
 * MF_FromFile.cc
 * This element read data from a file and creates routing layer
 * datagrams.
 * MF_FromFIle -> MF_Segmentor
 *  Created on: Jun 17, 2011
 *      Author: Kai Su
 *****************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/standard/scheduleinfo.hh>
#include <click/vector.hh>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include "mffromfile.hh"
#include "mffunctions.hh"
#include "mflogger.hh"


CLICK_DECLS

MF_FromFile::MF_FromFile() 
    : _task(this), _timer(&_task), _active(true),  _curr_chk_ID(1), _file(NULL),
      curr_file_offset(0), file_size(0), logger(MF_Logger::init()) {
}

MF_FromFile::~MF_FromFile() {
}

int MF_FromFile::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                   "SRC_GUID", cpkP+cpkM, cpUnsigned, &_src_GUID,
                   "DST_GUID", cpkP+cpkM, cpUnsigned, &_dst_GUID,
                   "SERVICE_ID", cpkP+cpkM, cpUnsigned, &_sid,
                   "FILE", cpkP+cpkM, cpString, &_file_to_send,
                   "CHK_SIZE", cpkP+cpkM, cpUnsigned, &_chk_size,
                   "DELAY", cpkP+cpkM, cpUnsigned, &_delay_sec,
                   "PKT_SIZE", cpkP+cpkM, cpUnsigned, &_pkt_size,
                   "MY_GUID_FILE", cpkP+cpkM, cpString, &_my_guid_file,
                   "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chk_manager,
                   cpEnd) < 0) {
    return -1;
  }
  return 0; 
}

int MF_FromFile::initialize(ErrorHandler *errh) {
  //check status of the file
  struct stat fs;
  if (stat(_file_to_send.c_str(), &fs)) {
    logger.log(MF_FATAL, "fromfile: failed file stat");
    exit(EXIT_FAILURE);
  }
  //number of chunks this file will be segmented into
  file_size = fs.st_size; 
  uint32_t num_chks = file_size/_chk_size + 1;
  logger.log(MF_DEBUG,
             "fromfile: File %s, size: %d, num chunks: %u",
             _file_to_send.c_str(), fs.st_size, num_chks);
  _file = fopen(_file_to_send.c_str(), "rb"); 
  if (!_file) {
    logger.log(MF_FATAL, "fromfile: failed fopen");
    exit(EXIT_FAILURE);
  }

  logger.log(MF_INFO, 
             "fromfile: service_id: %u, chunk size: %lu and pkt size: %u",
             _sid, _chk_size, _pkt_size);
  
  _my_guid = new GUID();
  if (_my_guid->init(_my_guid_file.c_str()) == 0) {     //read guid file successfully
    char buf[GUID_LOG_BUF_SIZE]; 
    logger.log(MF_DEBUG,
               "fromfile: Public Key File: %s, GUID: %s", 
               _my_guid_file.c_str(), _my_guid->to_log(buf));
  } else {
    logger.log(MF_WARN,
               "fromfile: cannot open my_guid_file: %s, " 
               "set src_guid: %u as my_guid",
               _my_guid_file.c_str(), _src_GUID);
  }

  //init task, but use timer to schedule after delay
  ScheduleInfo::initialize_task(this, &_task, false, errh);
  _timer.initialize(this);

  /* listen for downstream capacity available notifications */
  _signal = Notifier::downstream_full_signal(this, 0, &_task);
  _timer.schedule_after_msec(_delay_sec*1000);

  logger.log(MF_INFO, 
             "fromfile: Transmission begins in: %u sec", _delay_sec);
  return 0;
}

bool MF_FromFile::run_task(Task *) {
    
  if (!_active) {
    return false; //don't reschedule 
  }

  //check if downstream is full
  if (!_signal) {
    return false; //don't reschedule wait for notification
  }
   
  /* seek to file offset to resume */
  logger.log(MF_TIME, 
             "fromfile: Resuming file trxfr at offset: %lu",
             curr_file_offset);
  if (fseek(_file, curr_file_offset, SEEK_SET)) {
    logger.log(MF_FATAL, 
      "fromfile: failed seek to proper offset: %s", 
    strerror(errno));
    exit(EXIT_FAILURE);
  }

  WritablePacket * internal_pkt; // routing layer chunk packet
  Vector<Packet*> * pkt_vec; 
  Chunk *chunk = NULL;
  // how many packets in total
  uint32_t pkt_cnt = 0;
  uint32_t curr_chk_size = 0;
  char *payload;
  routing_header *rheader = NULL;
  transport_header *theader = NULL;

  while (!feof(_file)) {
    WritablePacket * pkt;

    //TODO: packet of max size being created even tho we may not fill it
    if (!(pkt = Packet::make(sizeof(click_ether), 0, _pkt_size, 0))) {
      logger.log(MF_FATAL, "fromfile: can't make packet!");
      exit(EXIT_FAILURE);
    }
    memset(pkt->data(), 0, pkt->length());
    /* Sync up with Agg implementation: pheader all fields in HBO,
     * other headers should be in NBO */
    hop_data_t * pheader = (hop_data_t *)(pkt->data());
    pheader->type = htonl(DATA_PKT);
    pheader->seq_num = htonl(pkt_cnt);//seq no starts at 0
    pheader->hop_ID = htonl(_curr_chk_ID);
    pkt_cnt++; 
    if (pkt_cnt == 1) {
      //first pkt of chunk -- add L3, L4 headers followed by file data
      // L3 header
      pkt_vec = new Vector<Packet *>(); 
      uint32_t extendedHeaderSize = 0; 
      rheader = new RoutingHeader( pkt->data() + HOP_DATA_PKT_SIZE );
      rheader->setVersion (1);
      switch (_sid) {
       case MF_ServiceID::SID_UNICAST:
        break; 
       case MF_ServiceID::SID_ANYCAST:
        break; 
       case MF_ServiceID::SID_MULTICAST:
        break;
       case MF_ServiceID::SID_MULTIHOMING:
        rheader->setServiceID(MF_ServiceID::SID_MULTIHOMING | 
                              MF_ServiceID::SID_EXTHEADER);         //unicast: 0, multihoming: 4
        break;
       default:
        logger.log(MF_ERROR,
                   "fromfile: unsupported Service ID, set to Unicast");
        rheader->setServiceID(MF_ServiceID::SID_UNICAST);
        break;
      }
      rheader->setUpperProtocol(TRANSPORT);
      rheader->setSrcGUID (_src_GUID);  //*(new GUID(_my_GUID) );
      rheader->setSrcNA(0);
      rheader->setDstGUID(_dst_GUID);  // *(new GUID(_dst_GUID)); 
      rheader->setDstNA(0);
 
      uint32_t extensionHeaderSize = 0; 
      if (rheader->hasExtensionHeader()) {
        if (rheader->getServiceID().isMultiHoming()) {
          logger.log(MF_DEBUG,
            "fromfile: serviceID is mutlihoming");
          ExtensionHeader *extHdr = new MultiHomingExtHdr(pkt->data() +
                        HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
          extensionHeaderSize = EXTHDR_MULTIHOME_MAX_SIZE;    //maximum size;
          //rheader->setPayloadOffset(l4_position);
          char buf[512];
          logger.log(MF_DEBUG,
                     "mutlihoming next header info: %s", extHdr->to_log(buf));
        } else {
          logger.log(MF_DEBUG, "chnk_src: no implementation");
        }
      }

      uint32_t payloadOffset = ROUTING_HEADER_SIZE + extensionHeaderSize;
      rheader->setPayloadOffset(payloadOffset);
      char buf[512];
      logger.log(MF_DEBUG,
                 "routing header info: %s", rheader->to_log(buf));
      
      theader = (transport_header*)(pkt->data() + HOP_DATA_PKT_SIZE + 
                                    payloadOffset); 
      
      //theader->chk_ID = htonl(_curr_chk_ID);
      /* chunk size is to be written */
      //theader->src_app_ID = 0;
      //theader->dst_app_ID = 0;
      //theader->reliability = 0;
      theader->chunkID = htonl(_curr_chk_ID);
      //theader->chunkSize = htonl(_chk_size);
      theader->srcTID = 0;
      theader->dstTID = 0;
      theader->msgID = 0;
      theader->msgNum = 0;
      theader->offset = 0;

      payload = (char*)(theader+1);

      uint32_t bytes_to_read = _pkt_size - (sizeof(click_ether)
                 + HOP_DATA_PKT_SIZE + payloadOffset
                 + TRANS_HEADER_SIZE);
      logger.log(MF_DEBUG, 
                 "fromfile: first packet data pld : %u", 
                 bytes_to_read); 
      //consider chunk boundary
      if ((bytes_to_read + curr_chk_size) > _chk_size) {
        bytes_to_read = _chk_size - curr_chk_size; 
      }

      int32_t bytes_read;
      if ((bytes_read = fread(payload, 1, bytes_to_read, _file)) < 0) {
        logger.log(MF_FATAL, "fromfile: err reading file");
        exit(EXIT_FAILURE);
      } else if(bytes_read == 0) {
        pkt->kill();
        pkt_cnt--;
        assert(feof(_file));
        break;
      } else {
        //set payload len
        pheader->pld_size = 
                   htonl(bytes_read + payloadOffset + TRANS_HEADER_SIZE);
        curr_chk_size += bytes_read;
      }

      // prepare a routing datagram
      internal_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
      if (!internal_pkt) {
        logger.log(MF_FATAL, "fromfile: cannot make chunk packet!"); 
        exit(EXIT_FAILURE); 
      }
      memset(internal_pkt->data(), 0, internal_pkt->length());
      pkt_vec->push_back( pkt ); 

    } else { 
      // data only pkt, no L3+ headers 
      uint32_t bytes_to_read = _pkt_size - (sizeof(click_ether)
                 + HOP_DATA_PKT_SIZE);
      //consider chunk boundary
      if ((bytes_to_read + curr_chk_size) > _chk_size) {
        bytes_to_read = _chk_size - curr_chk_size; 
      }
      payload = (char *)(pheader+1);
      int32_t bytes_read = fread(payload, 1, bytes_to_read, _file); 
      if (bytes_read < 0) {
        logger.log(MF_FATAL, "fromfile: err reading file");
        exit(EXIT_FAILURE);
      } else if (bytes_read == 0) {
        pkt->kill();
        pkt_cnt--;
        assert(feof(_file));
        break;
      } else {
        pheader->pld_size = htonl(bytes_read);
        curr_chk_size += bytes_read;
      } 
      pkt_vec->push_back(pkt); 
    }
    logger.log(MF_TRACE, 
      "fromfile: Created pkt #: %lu for chunk #: %lu rtg_id: %lu, "
      "curr size: %lu", 
      pkt_cnt, _curr_chk_ID, _curr_chk_ID, curr_chk_size);
    if (curr_chk_size == _chk_size) {
      /* aggregated 1 chunk */
      rheader->setChunkPayloadSize(curr_chk_size + TRANS_HEADER_SIZE);
      //theader->chk_size = htonl(curr_chk_size);
      theader->chunkSize = htonl(_chk_size);
      chunk = _chk_manager->alloc(pkt_vec, 0);
      chk_internal_trans_t * chk_trans = 
           (chk_internal_trans_t*)(internal_pkt->data());
      chk_trans->sid = htonl(rheader->getServiceID().to_int());
      chk_trans->chunk_id = chunk->getChunkID();
      logger.log(MF_DEBUG,
        "fromfile: create chunk internal trans msg sid: %u, chunk_id: %u",
         ntohl(chk_trans->sid), chk_trans->chunk_id);
      output(0).push(internal_pkt);

      logger.log(MF_INFO, 
        "fromfile: PUSHED ready chunk #: %u, size: %u, num_pkts: %u", 
      _curr_chk_ID, curr_chk_size, pkt_cnt);

      curr_chk_size = 0;
      pkt_cnt = 0;
      _curr_chk_ID++;

      if (!(curr_file_offset = ftell(_file))) {
        logger.log(MF_FATAL,
          "fromfile: ftell err, failed to save file offset");
          exit(EXIT_FAILURE);
      }
      _task.fast_reschedule();
      return true;
    }
  } //end file read loop
  /* push any last partial chunk */
  if (curr_chk_size) { 
    rheader->setChunkPayloadSize(curr_chk_size + TRANS_HEADER_SIZE); 
    //theader->chk_size = htonl(curr_chk_size);
    theader->chunkSize = htonl(_chk_size);
    chunk = _chk_manager->alloc(pkt_vec, 0);
    chk_internal_trans_t * chk_trans = 
                             (chk_internal_trans_t*)(internal_pkt->data());
    chk_trans->sid = htonl(rheader->getServiceID().to_int());
    chk_trans->chunk_id = chunk->getChunkID();
    logger.log(MF_DEBUG,
      "fromfile: create chunk internal passing message sid: %u, chunk_id: %u",
      ntohl(chk_trans->sid), chk_trans->chunk_id);

    output(0).push(internal_pkt);
    logger.log(MF_INFO, 
      "fromfile: PUSHED ready chunk #: %u, rtg_id:%u size: %u, num_pkts: %u",
      _curr_chk_ID, _curr_chk_ID , curr_chk_size, pkt_cnt);
  }
  _active = false;
  fclose(_file);
  return false; //done, no rescheduling
}

void MF_FromFile::add_handlers() {
  add_task_handlers(&_task, &_signal);
}

#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<pkt*>;
template class Vector<int>;
#endif

CLICK_DECLS
ELEMENT_LIBS(-lcrypto)
EXPORT_ELEMENT(MF_FromFile)
ELEMENT_REQUIRES(userlevel)
