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

/***********************************************************************
 * MF_VirtualLSAHandler.cc
 * Similar to mflsahandler used by GSTAR routing
 * Run_timer: Generates virtual network state packets (VIRTUAL_NETWORK_SA) defined in common/include/mfvirtualctrlmsg.hh
 * Handles two types of control messages:
 * 1. VIRTUAL_NETWORK_SA
 * 2. VIRTUAL_NETWORK_ASP

 *      Author: Aishwarya Babu
 ************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "mfvnlsahandler.hh"
#include "mffunctions.hh"
#include "mflogger.hh"
#include "mfchunkmanager.hh"
#include "mfvninfocontainer.hh"
#include "mfvnmanager.hh"


CLICK_DECLS

MF_VNLSAHandler::MF_VNLSAHandler(): _pkt_size(DEFAULT_PKT_SIZE), logger(MF_Logger::init()){
}

MF_VNLSAHandler::~MF_VNLSAHandler(){

}

int MF_VNLSAHandler::initialize(ErrorHandler *){
	return 0;
}

int MF_VNLSAHandler::configure(Vector<String> &conf, ErrorHandler *errh){
	if (cp_va_kparse(conf, this, errh,
			"MY_GUID", cpkP+cpkM, cpInteger, &my_guid,
			"VN_MANAGER", cpkP+cpkM, cpElement, &_vnManager,
			"CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
			cpEnd) < 0) {
		return -1;
	}
	return 0;
}

void MF_VNLSAHandler::callback(Timer *t, void *user_data){
	MF_VNLSAHandler *handler = ((timerInfo_t *)user_data)->handler;
	handler->run_timer(t,user_data);
}

/*!
 * Creates new chunks to send virtual network link status with respect to its neighbors
 * vlink cost obtained from virtual_nbr_tbl_p (Virtual Neighbor table)
 *
 */
void MF_VNLSAHandler::run_timer(Timer *t, void *user_data){
	timerInfo_t *info = (timerInfo_t *)user_data;
	logger.log(MF_DEBUG, "MF_VNLSAHandler:: Timer for VN %u", info->guid);
	assert (info->timer == t);
	if(timerMap.get(info->guid) == timerMap.default_value()) return;
	MF_VNInfoContainer *container = _vnManager->getVNContainer(info->guid);
	if(container == NULL || !container->isInitialized()) {
		logger.log(MF_WARN, "MF_VNLSAHandler:: VN %u does not exist", info->guid);
		return;
	}

	virtualNeighbor_map * virtualNeighborTable;
	virtualNeighborTable = container->getVNNeighborTable()->getVirtualNeighborMap();
	Vector<const virtualNeighbor_t*> vnbrs_vec;
	container->getVNNeighborTable()->getVirtualNeighbors(vnbrs_vec);

	int length = vnbrs_vec.size();
	logger.log(MF_DEBUG, "virtual_lsa_handler: Number of virtual neighbors: %d", length);

	if (length == 0) {
		logger.log(MF_DEBUG, "virtual_lsa_handler: No virtual neighbors yet!");
		//reschedule task after lsa interval
		info->timer->schedule_after_msec(VIRTUAL_LSA_PERIOD_MSECS);
		return;
	}

	//TODO I didn't get this, what is this size and what is it used for?
	int _chk_size = HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE + sizeof(VIRTUAL_NETWORK_SA) + length * 2 * sizeof(uint32_t);

	int _pkt_size = 0;
	//if condition to limit _pkt_size to DEFAULT_PKT_SIZE
	if(_chk_size < DEFAULT_PKT_SIZE) {
		_pkt_size = _chk_size +  sizeof(click_ether);
	} else {
		_pkt_size = DEFAULT_PKT_SIZE;
	}
	int headroom = sizeof(click_ether);

	routing_header *rheader = NULL;
	VirtualExtHdr * vextheader = NULL;
	Chunk *chunk = NULL;
	Vector<Packet*> *chk_pkts = NULL;

	//TODO: For loop needs to be changed to VIRTUAL_BROADCAST_GUID at some point
	for(int it = 0; it < length; it++)
	{
		//Some of the following lines may have to go outside the loop - figure memory alloc
		uint32_t pkt_cnt = 0;
		uint32_t curr_chk_size = 0;
		char *payload;
		rheader = NULL;
		vextheader = NULL;
		chunk = NULL;
		chk_pkts = NULL;

		WritablePacket *pkt = NULL;
		//In the chance that the number of neighbors are so large and the contents result in exceeding a single packet
		while (curr_chk_size < _chk_size) {

			if (pkt_cnt == 0) {   //if first pkt
				logger.log(MF_DEBUG, "virtual_lsa_handler: creating chunk's first packet with headers");

				pkt = Packet::make(headroom, 0, _pkt_size, 0);
				logger.log(MF_DEBUG, "virtual_lsa_handler: packet made \n");
				if (!pkt){
					logger.log(MF_CRITICAL, "virtual_lsa_handler: cannot make chunk packet!");
					return;
				}

				memset(pkt->data(), 0, pkt->length());

				//TODO: I do not think HOP management is necessary here
				/* HOP header */
				hop_data_t * pheader = (hop_data_t *)(pkt->data());
				pheader->type = htonl(DATA_PKT);
				pheader->seq_num = htonl(pkt_cnt);                    //starts at 0

				//L3  header
				rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE);
				rheader->setVersion(1);
				rheader->setServiceID(MF_ServiceID::SID_VIRTUAL |  MF_ServiceID::SID_EXTHEADER);
				logger.log(MF_DEBUG, "virtual_lsa_handler: set SID_VIRTUAL and SID_EXTHEADER");

				rheader->setUpperProtocol(VIRTUAL_CTRL);
				rheader->setDstGUID(vnbrs_vec[it]->true_guid);
				rheader->setDstNA(0);
				rheader->setSrcGUID(my_guid);
				rheader->setSrcNA(0);

				vextheader = new VirtualExtHdr(pkt->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
				vextheader->setVirtualNetworkGUID(container->getVirtualNetworkGuid());
				vextheader->setVirtualSourceGUID(container->getMyVirtualGuid());
				vextheader->setVirtualDestinationGUID(vnbrs_vec[it]->virtual_guid);
				logger.log(MF_DEBUG, "virtual_lsa_handler: set destination guid %u", vnbrs_vec[it]->virtual_guid);

				VIRTUAL_NETWORK_SA * virtual_network_sa_info;
				virtual_network_sa_info = (VIRTUAL_NETWORK_SA *)(pkt->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE);
				virtual_network_sa_info->type = VIRTUAL_CTRL_PKT;
				virtual_network_sa_info->senderVirtual = container->getMyVirtualGuid();
				virtual_network_sa_info->seq = container->getLspSeqNo();
				virtual_network_sa_info->nodeMetric = 0; //Sets own value of nodeMetric
				virtual_network_sa_info->size = length;

				logger.log(MF_DEBUG, "virtual_lsa_handler: populating the virtual lsa portion");

				uint32_t * vnbr_guid = (uint32_t *) (virtual_network_sa_info + 1);
				for(int nbr_iter = 0; nbr_iter < length; nbr_iter++) {
					*vnbr_guid = vnbrs_vec[nbr_iter]->virtual_guid;
					vnbr_guid++;
				}

				uint32_t * vlink_cost = vnbr_guid;
				for(int nbr_iter = 0; nbr_iter < length; nbr_iter++) {
					*vlink_cost = vnbrs_vec[nbr_iter]->vlinkCost;
					vlink_cost++;
				}

				logger.log(MF_DEBUG, "virtual_lsa_handler: payload sizing");

				uint32_t payloadOffset = ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE;
				rheader->setPayloadOffset(payloadOffset);
				char buf[512];
				logger.log(MF_DEBUG, "routing header info: %s", rheader->to_log(buf));
				chk_pkts = new Vector<Packet*> ();
				logger.log(MF_DEBUG, "virtual_lsa_handler: new vector declared");
				//                curr_chk_size += _pkt_size - (sizeof(click_ether) + payloadOffset);
				curr_chk_size += _pkt_size - (sizeof(click_ether));
				pheader->pld_size = htonl(_pkt_size - (sizeof(click_ether) + HOP_DATA_PKT_SIZE));

				container->setLspSeqNo(container->getLspSeqNo()+1);
			}
			//TODO so this is never used?
			else {
				logger.log(MF_DEBUG, "virtual_lsa_handler: creating packet %d", pkt_cnt);
				uint32_t remaining_bytes = _chk_size - curr_chk_size;
				uint32_t bytes_to_send = _pkt_size;
				// if remaining data size is less than data pkt payload
				if (remaining_bytes < (_pkt_size - sizeof(click_ether) - HOP_DATA_PKT_SIZE)) {
					bytes_to_send = remaining_bytes + sizeof(click_ether) +  HOP_DATA_PKT_SIZE;
				}

				uint32_t pkt_payload = bytes_to_send - sizeof(click_ether) -  HOP_DATA_PKT_SIZE;
				pkt = Packet::make(sizeof(click_ether), 0, bytes_to_send, 0);
				if (!pkt) {
					logger.log(MF_FATAL, "virtual_lsa_handler: can't make packet!");
					exit(EXIT_FAILURE);
				}
				memset(pkt->data(), 0, pkt->length());

				/* HOP header */
				hop_data_t * pheader = (hop_data_t *)(pkt->data());
				pheader->type = htonl(DATA_PKT);
				pheader->seq_num = htonl(pkt_cnt);
				//starts at 0
				pheader->pld_size = htonl(pkt_payload);
				curr_chk_size += pkt_payload;
			}
			logger.log(MF_DEBUG, "virtual_lsa_handler: to push packets into vector");
			chk_pkts->push_back(pkt);
			++pkt_cnt;

			delete rheader;
			delete vextheader;

			logger.log(MF_INFO, "virtual_lsa_handler: Size of click_ether: %u, _pkt_size: %u _chk_size: %u", sizeof(click_ether), _pkt_size, _chk_size);
			pkt = NULL;
		}
		//chunk packet for sending
		WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
		if (!chk_pkt) {
			logger.log(MF_CRITICAL, "virtual_lsa_handler: cannot make internal chunk msg packet!");
			exit(EXIT_FAILURE);
		}
		memset(chk_pkt->data(), 0, chk_pkt->length());

		chunk = _chunkManager->alloc(chk_pkts, ChunkStatus::ST_INITIALIZED||ChunkStatus::ST_COMPLETE);
		chk_internal_trans_t *chk_trans = ( chk_internal_trans_t*) chk_pkt->data();
		chk_trans->sid = htonl(chunk->getServiceID().to_int());
		chk_trans->chunk_id = chunk->getChunkID();
		output(0).push(chk_pkt);

	}
	//reschedule task after lsa interval
	info->timer->schedule_after_msec(VIRTUAL_LSA_PERIOD_MSECS);
}

void MF_VNLSAHandler::addVNTimer(uint32_t vnGUID){
	logger.log(MF_DEBUG, "MF_VNLSAHandler:: Adding timer for VN %u", vnGUID);
	if(timerMap.get(vnGUID) == timerMap.default_value()){
		timerInfo_t *info = new timerInfo_t();
		Timer *newTimer = new Timer(&callback, (void*)info);
		info->handler = this;
		info->timer = newTimer;
		info->guid = vnGUID;
		timerMap.set(vnGUID, info);
		info->timer->initialize(this);
		info->timer->schedule_after_msec(VIRTUAL_LSA_PERIOD_MSECS);
	} else {
		logger.log(MF_WARN, "MF_VNLSAHandler:: Timer for VN %u already existing", vnGUID);
	}
}

void MF_VNLSAHandler::removeVNTimer(uint32_t vnGUID){
	timerInfo_t *info = timerMap.get(vnGUID);
	if(info != timerMap.default_value()){
		info->timer->unschedule();
		timerMap.erase(vnGUID);
		delete info;
	}
}


void MF_VNLSAHandler::push (int port, Packet * pkt){
	if (port == 0) {
		// lsa message
		chk_internal_trans_t* chk_trans = (chk_internal_trans_t*)pkt->data();
		uint32_t sid = chk_trans->sid;
		uint32_t chunk_id = chk_trans->chunk_id;
		Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
		if (!chunk) {
			logger.log(MF_ERROR, "to_file: Got a invalid chunk from ChunkManager");
			return;
		}

		RoutingHeader* rheader = chunk->routingHeader();
		uint32_t src = rheader->getSrcGUID().to_int();

		logger.log(MF_DEBUG, "virtual_lsa_handler: source guid %u ", src);

		if(src == my_guid) {
			pkt->kill();
			return;
		}

		if(chunk->getUpperProtocol() == VIRTUAL_CTRL) {
			handleVirtualLSA(chunk);
			//forwardVirtualLSA(chunk);
		} else if(chunk->getUpperProtocol() == VIRTUAL_ASP) {
			handleVirtualASP(chunk);
		}
	} else {
		logger.log(MF_FATAL,
				"virtual_lsa_handler: Packet received on unsupported port");
		exit(-1);
	}
}

/*!
 * Virtual network state packet received from other virtual routers are inserted into the virtual neighbor table
 * The received Virtual NSP / virtual LSP are forwarded to the neighbors
 *
 * TODO: Currently new chunks are made for every virtual NSP that needs to be forwarded
	Need to simply re populate the virtual and routing headers and forward 	
 */
void MF_VNLSAHandler::handleVirtualLSA(Chunk *chunk){
	logger.log(MF_DEBUG, "virtual_lsa_handler: received a virtual lsa packet with chunk id: %u", chunk->getChunkID());
	int lsa_flag;
	WritablePacket* pkt = NULL;
	RoutingHeader* rheader = NULL;
	VirtualExtHdr* vextheader = new VirtualExtHdr(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
	Vector<Packet*> *chk_pkts = NULL;
	MF_VNInfoContainer *container = _vnManager->getVNContainer(vextheader->getVirtualNetworkGUID().to_int());
	lsa_flag = container->getVNNeighborTable()->insertVirtualNSP(chunk);

	//TODO fix all this memory mismanagement and error checking
	delete vextheader;

	if(lsa_flag == 1) {
		container->getVNNeighborTable()->req_vnbrt_update();
		container->getVNRoutingTable()->req_vrt_update();

		logger.log(MF_DEBUG, "virtual_lsa_handler: Need to broadcast at virtual level by changing destination ");
		Vector<const virtualNeighbor_t*> vnbrs_vec;
		container->getVNNeighborTable()->getVirtualNeighbors(vnbrs_vec);

		int headroom = sizeof(click_ether);
		logger.log(MF_DEBUG, "virtual_lsa_handler: Next loop to forward VNSA to neighbors numNeighbors: %d", vnbrs_vec.size());

		for(int it = 0; it < vnbrs_vec.size(); it++) {
			rheader = NULL;
			vextheader = NULL;
			chk_pkts = NULL;

			pkt = Packet::make(headroom, chunk->allPkts()->at(0)->data(), chunk->allPkts()->at(0)->length(), 0);
			logger.log(MF_DEBUG, "virtual_lsa_handler: packet made \n");
			if (!pkt){
				logger.log(MF_CRITICAL, "virtual_lsa_handler: cannot make internal chunk msg packet!");
				return;
			}

			//ROUTING HEADER
			rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE);
			uint32_t guid_dest1 = rheader->getDstGUID().to_int();
			uint32_t guid_src1 = rheader->getSrcGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: src guid old : %u ", guid_src1);
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: dest guid old : %u ", guid_dest1);
			rheader->setSrcGUID(my_guid);
			rheader->setSrcNA(0);
			rheader->setDstGUID(vnbrs_vec[it]->true_guid);
			rheader->setDstNA(0);
			guid_dest1 = rheader->getDstGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: dest guid new and neighbor :%u %u ", guid_dest1, vnbrs_vec[it]->true_guid);

			//VIRTUAL EXT HEADER
			vextheader = new VirtualExtHdr(pkt->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);

			//Dont send back to where it came from
			uint32_t virtual_src = vextheader->getVirtualSourceGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual src guid old : %u ", virtual_src);
			if(vnbrs_vec[it]->virtual_guid == virtual_src) {
				//TODO: Avoid creating the rheader in the first place by moving the ROUTING HEADER section after this if condition
				logger.log(MF_DEBUG, "virtual_lsa_handler: To prevent sending packet back to the virtual node it came from");
				delete rheader;
				delete vextheader;
				pkt->kill();
				continue;
			}

			//Dont send back to where it came from - update virtual source guid
			vextheader->setVirtualSourceGUID(container->getMyVirtualGuid());

			guid_dest1 = vextheader->getVirtualDestinationGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual dest guid old : %u ", guid_dest1);
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual src guid new : %u ", container->getMyVirtualGuid());
			vextheader->setVirtualDestinationGUID(vnbrs_vec[it]->virtual_guid);
			guid_dest1 = vextheader->getVirtualDestinationGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual dest guid new : %u ", guid_dest1);

			//NEW CHUNK
			chk_pkts = new Vector<Packet*> ();
			chk_pkts->push_back(pkt);

			for(int pkt_iter = 1; pkt_iter < chunk->allPkts()->size(); pkt_iter++) {
				WritablePacket* pkt_copy = NULL;
				pkt_copy = Packet::make(headroom, chunk->allPkts()->at(pkt_iter)->data(), chunk->allPkts()->at(pkt_iter)->length(), 0);
				chk_pkts->push_back(pkt_copy);
			}

			WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
			if (!chk_pkt) {
				logger.log(MF_FATAL, "virtual_lsa_handler: can't make chunk packet!");
				exit(EXIT_FAILURE);
			}
			memset(chk_pkt->data(), 0, chk_pkt->length());

			Chunk * chunk1 = NULL;
			chunk1 = _chunkManager->alloc(chk_pkts, ChunkStatus::ST_INITIALIZED||ChunkStatus::ST_COMPLETE);
			chunk1->clearAddressOptions(); //clear the network address options for the chunk
			chk_internal_trans_t *chk_trans1 = ( chk_internal_trans_t*) chk_pkt->data();
			chk_trans1->sid = htonl(chunk1->getServiceID().to_int());
			chk_trans1->chunk_id = chunk1->getChunkID();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: chunk id: %u ", chk_trans1->chunk_id);

			delete vextheader;
			delete rheader;

			output(0).push(chk_pkt);
		}
	} else {
		//TODO:send chunk to sink
		//pkt->kill();
	}
}

/*!
 * Virtual application state packet received from the end points/ application/ destnation
 * Information inserted into the Virtual routing table
 * The received Virtual ASPs are forwarded to the neighbors
 *
 * TODO: Currently new chunks are made for every virtual ASP that needs to be forwarded
	Need to simply re populate the virtual and routing headers and forward 	
 */
void MF_VNLSAHandler::handleVirtualASP(Chunk *chunk){

	//TODO fix all this memory mismanagement and error checking

	logger.log(MF_DEBUG, "virtual_lsa_handler: received a virtual asp packet with chunk id: %u", chunk->getChunkID());
	int lsa_flag;
	RoutingHeader* rheader = chunk->routingHeader();
	VirtualExtHdr* vextheader = new VirtualExtHdr(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
	Vector<Packet*> *chk_pkts = NULL;
	MF_VNInfoContainer *container = _vnManager->getVNContainer(vextheader->getVirtualNetworkGUID().to_int());
	WritablePacket* pkt = NULL;

	VIRTUAL_NETWORK_ASP * virtual_network_asp_info;
	virtual_network_asp_info = (VIRTUAL_NETWORK_ASP*)(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + rheader->getPayloadOffset() + TRANS_HEADER_SIZE);
	lsa_flag = container->getVNRoutingTable()->insertVirtualASP(virtual_network_asp_info);
	delete rheader;

	//Forwarding to neighbors and requesting table updates
	if(lsa_flag == 1) {
		container->getVNNeighborTable()->req_vnbrt_update();
		container->getVNRoutingTable()->req_vrt_update();

		logger.log(MF_DEBUG, "virtual_lsa_handler: Need to broadcast at virtual level by changing destination ");
		Vector<const virtualNeighbor_t*> vnbrs_vec;
		container->getVNNeighborTable()->getVirtualNeighbors(vnbrs_vec);

		int headroom = sizeof(click_ether);
		logger.log(MF_DEBUG, "virtual_lsa_handler: Next loop to forward Virtual ASP to neighbors numNeighbors: %d", vnbrs_vec.size());



		//TODO: Dont send back to where it came from - update virtual source guid
		for(int it = 0; it < vnbrs_vec.size(); it++) {
			rheader = NULL;
			vextheader = NULL;
			chk_pkts = NULL;


			pkt = Packet::make(headroom, chunk->allPkts()->at(0)->data(), chunk->allPkts()->at(0)->length(), 0);
			logger.log(MF_DEBUG, "virtual_lsa_handler: packet made \n");
			if (!pkt){
				logger.log(MF_CRITICAL, "virtual_lsa_handler: cannot make internal chunk msg packet!");
				return;
			}

			//ROUTING HEADER
			rheader = new RoutingHeader(pkt->data() + HOP_DATA_PKT_SIZE);
			uint32_t guid_dest1 = rheader->getDstGUID().to_int();
			uint32_t guid_src1 = rheader->getSrcGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: src guid old : %u ", guid_src1);
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: dest guid old : %u ", guid_dest1);

			rheader->setSrcGUID(my_guid);
			rheader->setSrcNA(0);
			rheader->setDstGUID(vnbrs_vec[it]->true_guid);
			rheader->setDstNA(0);
			guid_dest1 = rheader->getDstGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: dest guid new and neighbor :%u %u ", guid_dest1, vnbrs_vec[it]->true_guid);

			//VIRTUAL EXT HEADER
			vextheader = new VirtualExtHdr(pkt->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
			char buf[512];
			logger.log(MF_DEBUG, "virtual routing header info: %s", vextheader->to_log(buf));

			uint32_t virtual_src = vextheader->getVirtualSourceGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual src guid old : %u ", virtual_src);
			if(vnbrs_vec[it]->virtual_guid == virtual_src) {
				//TODO: Avoid creating the rheader in the first place by moving the ROUTING HEADER section after this if condition
				logger.log(MF_DEBUG, "virtual_lsa_handler: To prevent sending packet back to the virtual node it came from");
				delete rheader;
				delete vextheader;
				pkt->kill();
				continue;
			}

			guid_dest1 = vextheader->getVirtualDestinationGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual dest guid old : %u ", guid_dest1);
			vextheader->setVirtualSourceGUID(container->getMyVirtualGuid());
			vextheader->setVirtualDestinationGUID(vnbrs_vec[it]->virtual_guid);
			guid_dest1 = vextheader->getVirtualDestinationGUID().to_int();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: virtual dest guid new : %u ", guid_dest1);

			//NEW CHUNK
			chk_pkts = new Vector<Packet*> ();
			chk_pkts->push_back(pkt);

			for(int pkt_iter = 1; pkt_iter < chunk->allPkts()->size(); pkt_iter++) {
				WritablePacket* pkt_copy = NULL;
				pkt_copy = Packet::make(headroom, chunk->allPkts()->at(pkt_iter)->data(), chunk->allPkts()->at(pkt_iter)->length(), 0);
				chk_pkts->push_back(pkt_copy);
			}

			WritablePacket *chk_pkt = Packet::make(0, 0, sizeof(chk_internal_trans_t), 0);
			if (!chk_pkt) {
				logger.log(MF_FATAL, "virtual_lsa_handler: can't make chunk packet!");
				exit(EXIT_FAILURE);
			}
			memset(chk_pkt->data(), 0, chk_pkt->length());

			Chunk * chunk1 = NULL;
			chunk1 = _chunkManager->alloc(chk_pkts, ChunkStatus::ST_INITIALIZED||ChunkStatus::ST_COMPLETE);
			chunk1->clearAddressOptions(); //clear the network address options for the chunk
			chk_internal_trans_t *chk_trans1 = ( chk_internal_trans_t*) chk_pkt->data();
			chk_trans1->sid = htonl(chunk1->getServiceID().to_int());
			chk_trans1->chunk_id = chunk1->getChunkID();
			logger.log(MF_DEBUG, "virtual_lsa_handler: handleVirtualLSA: chunk id: %u ", chk_trans1->chunk_id);


			delete vextheader;
			delete rheader;
			output(0).push(chk_pkt);
		}
	} else {
		//TODO: send chunk to sink
		//pkt->kill();
	}
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_VNLSAHandler)
