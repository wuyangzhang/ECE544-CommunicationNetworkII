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
 * MF_VirtualNeighborTable.cc
 *
 * Keeps track of virtual network state packets received in virtualNSP_map
 * Periodically updates virtual neighbor table with information received in the virtual network state packet
 * Information in this table is used by the MF_VirtualRoutingTable.cc to update routing information
 *

 *      Author: Aishwarya Babu
 ************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <pthread.h>

#include <vector>

#include "mffunctions.hh"

#include "mfvnneighbortablecompute.hh"
#include "mfvnneighbortable.hh"
#include "mfvnmanager.hh"
#include "mfvninfocontainer.hh"
#define TMP_LINE_SIZE 1000

/*
Reference:
http://www.orbit-lab.org/browser/mf/router/click/elements/gstar/mfroutingtable.cc?rev=0d66f570a8661ebb2e29238d220ace3e33c1f0bd

 */

CLICK_DECLS
MF_VNNeighborTableCompute::MF_VNNeighborTableCompute() : logger(MF_Logger::init()) {
	virtual_nsp_all = new virtualNSP_map();
}

MF_VNNeighborTableCompute::~MF_VNNeighborTableCompute() {
}

int MF_VNNeighborTableCompute::initialize(ErrorHandler *){
	return 0;
}

int MF_VNNeighborTableCompute::configure(Vector<String> &conf, ErrorHandler *errh) {
	if (cp_va_kparse(conf, this, errh,
			"ROUTING_TABLE", cpkP+cpkM, cpElement, &rtg_tbl_p,
			"NEIGHBOR_TABLE", cpkP+cpkM, cpElement, &nbr_tbl_p,
			"VN_MANAGER", cpkP+cpkM, cpElement, &_vnManager,
			cpEnd) < 0) {
		return -1;
	}
	return 0;
}  

void MF_VNNeighborTableCompute::callback(Timer *t, void *user_data){
	MF_VNNeighborTableCompute *handler = ((timerInfo_t *)user_data)->handler;
	handler->run_timer(t,user_data);
}

void MF_VNNeighborTableCompute::run_timer(Timer *t, void *user_data) {
	timerInfo_t *info = (timerInfo_t *)user_data;
	logger.log(MF_DEBUG, "MF_VNNeighborTableCompute:: Timer for VN %u", info->guid);
	assert (info->timer == t);
	if(timerMap.get(info->guid) == timerMap.default_value()) return;
	MF_VNInfoContainer *container = _vnManager->getVNContainer(info->guid);
	if(container == NULL || !container->isInitialized()) {
		logger.log(MF_WARN, "MF_VNNeighborTableCompute:: VN %u does not exist", info->guid);
		return;
	}
	MF_VNNeighborTable *neighborTable = container->getVNNeighborTable();
	if (neighborTable->isVnbrtUpdateFlag() || neighborTable->getVnbrtSkippedUpdates() >= MAX_VNBRT_SKIPPED_UPDATES) {
		neighborTable->setVnbrtUpdateFlag(false);
		neighborTable->setVnbrtSkippedUpdates(0);
		vnbrt_update(container);
		neighborTable->printNSPmap();
		neighborTable->print();
	} else {
		neighborTable->setVnbrtSkippedUpdates(neighborTable->getVnbrtSkippedUpdates()+1);
	}
	info->timer->schedule_after_msec(VNBRT_UPDATE_PERIOD_MSECS);
}



void MF_VNNeighborTableCompute::addVNTimer(uint32_t vnGUID){
	logger.log(MF_DEBUG, "MF_VNNeighborTableCompute:: Adding timer for VN %u", vnGUID);
	if(timerMap.get(vnGUID) == timerMap.default_value()){
		timerInfo_t *info = new timerInfo_t();
		Timer *newTimer = new Timer(&callback, (void*)info);
		info->handler = this;
		info->timer = newTimer;
		info->guid = vnGUID;
		timerMap.set(vnGUID, info);
		info->timer->initialize(this);
		info->timer->schedule_after_msec(VNBRT_UPDATE_PERIOD_MSECS);
	} else {
		logger.log(MF_WARN, "MF_VNNeighborTableCompute:: Timer for VN %u already existing", vnGUID);
	}
}

void MF_VNNeighborTableCompute::removeVNTimer(uint32_t vnGUID){
	timerInfo_t *info = timerMap.get(vnGUID);
	if(info != timerMap.default_value()){
		info->timer->unschedule();
		timerMap.erase(vnGUID);
		delete info;
	}
}

uint32_t MF_VNNeighborTableCompute::getVirtualPathLength(uint32_t trueDestGUID) {
	//TODO uint32_t trueDestGUID = vguid_map->getTrueGUID(virtualDestGUID);

	uint32_t virtualPathLength;
	virtualPathLength = rtg_tbl_p->get_path_length(trueDestGUID);

	return virtualPathLength;
}

uint32_t MF_VNNeighborTableCompute::getAggregatedSETT(uint32_t trueDestGUID) {
	//TODO uint32_t trueDestGUID = vguid_map->getTrueGUID(virtualDestGUID);
	uint32_t aggregatedSETT = 0;

	uint32_t truePath[MAX_PATH_LENGTH];

	forward_table = rtg_tbl_p->getForwardTable();
	lsa_all = rtg_tbl_p->getLSAMap();

	forward_map::iterator it = forward_table->find(trueDestGUID);
	if (it != forward_table->end()) {
		int pathLength = it.value()->path_length;
		//TODO change print statement
		logger.log(MF_INFO, "virtual_nbr_tbl: Found virtual guid: %u in forwardTable; path length: %d", trueDestGUID, pathLength);
		for(int counter = 0; counter < pathLength; counter++) {
			logger.log(MF_DEBUG, "virtual_nbr_tbl: \t \t \t path %u : %u", counter, it.value()->path[counter]);
		}

		//path seems to be reverse stored
		//Adding SETT value from my_guid to neighbor
		neighbor_t nbr;

		if(nbr_tbl_p->get_neighbor(it.value()->path[pathLength - 1], nbr)) {
			aggregatedSETT += nbr.s_ett;
			logger.log(MF_DEBUG, "\t \t searching nbr in nbr table %u", nbr.guid);
		}

		//Adding SETTs starting from neighbor along the path to the destination
		for(int i = 1; i < pathLength; i++) {
			uint32_t curr_guid = it.value()->path[pathLength - i];
			uint32_t next_guid = it.value()->path[pathLength - i - 1];
			//find sett between curr_guid and next_guid -> from curr_guid point of view
			//            logger.log(MF_INFO, "\t \t curr guid: %u, next guid: %u", curr_guid, next_guid);
			uint32_t sett_val;
			//    LSA_map::iterator iter_lsa = lsa_all->find(next_guid);
			LSA_map::iterator iter_lsa = lsa_all->find(curr_guid);
			if (iter_lsa != lsa_all->end()) {
				for (int j = 0; j < iter_lsa.value()->len; j++) {
					//                    logger.log(MF_DEBUG, "\t \t parsing lsa_all to find neighbr %u", iter_lsa.value()->ip_n[j]);
					if(iter_lsa.value()->ip_n[j] == next_guid) {
						logger.log(MF_DEBUG, "\t \t SETT from %u to %u is %u", curr_guid, next_guid, iter_lsa.value()->sett[j]);
						aggregatedSETT += iter_lsa.value()->sett[j];
					}
				}

			}
		}
	}
	return aggregatedSETT;
}

void MF_VNNeighborTableCompute::vnbrt_update(MF_VNInfoContainer *container) {
	logger.log(MF_DEBUG, "MF_VNNeighborTableCompute:: Updating neighbor table for VN %u", container->getVirtualNetworkGuid());
	//for all virtual neighbors
	virtualNeighbor_map *virtualNeighborTable = container->getVNNeighborTable()->getVirtualNeighborMap();
	for(virtualNeighbor_map::iterator it = virtualNeighborTable->begin(); it != virtualNeighborTable->end(); it++) {
		// Use getVirtuaPathLength / getAggregatedSETT

		uint32_t vlinkCost;
		switch(container->getVNGUIDMap()->getAlgorithmIndicator()) {
		case ALGO_1:
			logger.log(MF_DEBUG,"Algorithm : %u", ALGO_1);
			vlinkCost = getVirtualPathLength(container->getVNGUIDMap()->getTrueGUID(it.key()));
			break;
		case ALGO_2:
			logger.log(MF_DEBUG,"Algorithm : %u", ALGO_2);
			vlinkCost = getAggregatedSETT(container->getVNGUIDMap()->getTrueGUID(it.key()));
			break;
		}



		if(vlinkCost > MAX_VLINK_COST) {
			vlinkCost = MAX_VLINK_COST;
		}

		it.value()->vlinkCost = vlinkCost;
		virtualNSP_map::iterator iter_vnsp = virtual_nsp_all->find(it.key());
		//populating the virtual neighbor table which will be used to update the VNSP
		if(iter_vnsp == virtual_nsp_all->end()) {
			it.value()->nodeMetric = 0;
		} else {
			it.value()->nodeMetric = iter_vnsp.value()->nodeMetric;
		}
	}
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_VNNeighborTableCompute)
