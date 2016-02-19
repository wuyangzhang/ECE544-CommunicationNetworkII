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

#include "mfvirtualchunksource.hh"


CLICK_DECLS

MF_VirtualChunkSource::MF_VirtualChunkSource()
    : _task(this), _timer(&_task), _active(true), _start_msg_sent(false), 
    _curr_chk_ID(1), _curr_chk_cnt(0), 
    _pkt_size(DEFAULT_PKT_SIZE), _chk_size(DEFAULT_PKT_SIZE), _chk_intval_msec(1000), _dst_NAs_str(), 
    _delay_sec(10), logger(MF_Logger::init()) {
        _dst_NAs = new Vector<NA>();
    }

MF_VirtualChunkSource::~MF_VirtualChunkSource() {

}

int MF_VirtualChunkSource::configure(Vector<String> &conf, ErrorHandler *errh) {

    if (cp_va_kparse(conf, this, errh,
                "SRC_GUID", cpkP+cpkM, cpUnsigned, &_src_GUID,
                "DST_GUID", cpkP+cpkM, cpUnsigned, &_dst_GUID,
                "VIRTUAL_NETWORK_GUID", cpkP+cpkM, cpUnsigned, &_virtual_network_GUID,
                "VIRTUAL_SRC_GUID", cpkP+cpkM, cpUnsigned, &_virtual_src_GUID,
                "VIRTUAL_DST_GUID", cpkP+cpkM, cpUnsigned, &_virtual_dst_GUID,
                "ROUTER_STATS", cpkP+cpkM, cpElement, &_routerStats,
                "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                cpEnd) < 0) {
        return -1;
    }
    return 0; 
}

int MF_VirtualChunkSource::initialize(ErrorHandler * errh) {
    //check pkt size and chk size config
    if (_pkt_size > _chk_size) {
        logger.log(MF_ERROR, 
                "chnk_src: configuration error!"); 
    }
    logger.log(MF_INFO, 
            "chnk_src: Sending chunks of size:  %u, MTU: %u,interval: %u msecs", _chk_size, _pkt_size, _chk_intval_msec);

    //init task, but use timer to schedule after delay
    ScheduleInfo::initialize_task(this, &_task, false, errh);
    _timer.initialize(this);
    /* listen for downstream capacity available notifications */
    //    _signal = Notifier::downstream_full_signal(this, 0, &_task);

    _timer.schedule_after_msec(_delay_sec * 1000);
    logger.log(MF_INFO, 
            "chnk_src: Transmission begins in: %u sec", _delay_sec);
    return 0;
}

bool MF_VirtualChunkSource::run_task(Task *) {

    if (!_active) {
        return false;    //don't reschedule 
    }

    //check if downstream is full
    /*    if (!_signal) {
          return false; //don't reschedule wait for notification
          }*/

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

    uint32_t pkt_cnt = 0;
    uint32_t curr_chk_size = 0;
    char *payload;
    routing_header *rheader = NULL;
    VirtualExtHdr *vextheader = NULL;
    Chunk *chunk = NULL;
    Vector<Packet*> *chk_pkts = 0; 
    bool empty_pkt;

    _chk_size = HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE;
    _pkt_size = _chk_size;

    // prepare a convenience Chunk pkt for Click pipeline
    WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0); 
    if (!chk_pkt) {
        logger.log(MF_FATAL, "vchnk_src: can't make chunk packet!");
        exit(EXIT_FAILURE);
    }
    memset(chk_pkt->data(), 0, chk_pkt->length());

    WritablePacket *pkt = NULL;   
    while (curr_chk_size < _chk_size) {

        if (pkt_cnt == 0) {   //if first pkt
            pkt = Packet::make(sizeof(click_ether), 0, _pkt_size, 0); 
            if (!pkt) {
                logger.log(MF_FATAL, "vchnk_src: can't make packet!");
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
            rheader->setServiceID(MF_ServiceID::SID_VIRTUAL | MF_ServiceID::SID_EXTHEADER);
            logger.log(MF_DEBUG, "vchnk_src: set SID_VIRTUAL and SID_EXTHEADER"); 

            //use transport protocal
            rheader->setUpperProtocol(VIRTUAL_DATA); 
            rheader->setDstGUID(_dst_GUID);
            //if dst NA is defined in config file
            rheader->setDstNA(0);
            rheader->setSrcGUID(_src_GUID);
            rheader->setSrcNA(0);

            vextheader = new VirtualExtHdr(pkt->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
            vextheader->setVirtualNetworkGUID(_virtual_network_GUID); 
            vextheader->setVirtualSourceGUID(_virtual_src_GUID); 
            vextheader->setVirtualDestinationGUID(_virtual_dst_GUID); 
            logger.log(MF_DEBUG, "virtual_lsa_handler: set destination guid %u", _virtual_dst_GUID);

            uint32_t payloadOffset = ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE; 
            rheader->setPayloadOffset(payloadOffset);
            char buf[512];
            logger.log(MF_DEBUG,
                    "routing header info: %s", rheader->to_log(buf));
            chk_pkts = new Vector<Packet*> ();              
            //L4  header
            curr_chk_size += _pkt_size - (sizeof(click_ether) + payloadOffset);
            pheader->pld_size = htonl(_pkt_size - (sizeof(click_ether) + HOP_DATA_PKT_SIZE));
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
    chunk = _chunkManager->alloc(chk_pkts, ChunkStatus::ST_INITIALIZED||ChunkStatus::ST_COMPLETE); 
    chk_internal_trans_t *chk_trans = ( chk_internal_trans_t*) chk_pkt->data(); 
    chk_trans->sid = htonl(chunk->getServiceID().to_int());
    chk_trans->chunk_id = chunk->getChunkID(); 

    output(0).push(chk_pkt);
    _curr_chk_cnt++;
    logger.log(MF_INFO, 
            "chnk_src: PUSHED ready chunk #: %u, chunk_id: %u size: %u, num_pkts: %u", 
            _curr_chk_ID, chunk->getChunkID(), _chk_size, pkt_cnt);
    _curr_chk_ID++;


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
/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<pkt*>;
template class Vector<int>;
#endif
 */
CLICK_ENDDECLS
EXPORT_ELEMENT(MF_VirtualChunkSource)
ELEMENT_REQUIRES(userlevel)
