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
 * MF_VirtualRoutingTable.cc
 *
 * Performs djikstra periodically using the link metric
 * Maintain the node metric received using VIRTUAL_NETWORK_ASP
 * Used by the MF_VirtualDataForwarding.cc element

 *      Author: Aishwarya Babu
 ************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <pthread.h>
#include <click/hashtable.hh>

#include <vector>

#include "mffunctions.hh"

#include "mfvnroutingtable.hh"
#include "mfvnroutingtablecompute.hh"
#include "mfvninfocontainer.hh"
#include "mfvnmanager.hh"

/*
Reference:
http://www.orbit-lab.org/browser/mf/router/click/elements/gstar/mfroutingtable.cc?rev=0d66f570a8661ebb2e29238d220ace3e33c1f0bd

 */

CLICK_DECLS
MF_VNRoutingTableCompute::MF_VNRoutingTableCompute() : logger(MF_Logger::init()){

}

MF_VNRoutingTableCompute::~MF_VNRoutingTableCompute() {
}

int MF_VNRoutingTableCompute::initialize(ErrorHandler *handler){
	return 0;
}

int MF_VNRoutingTableCompute::configure(Vector<String> &conf, ErrorHandler *errh) {
	if (cp_va_kparse(conf, this, errh,
			"VN_MANAGER", cpkP+cpkM, cpElement, &_vnManager,
			cpEnd) < 0) {
		return -1;
	}
	return 0;
} 

void MF_VNRoutingTableCompute::callback(Timer *t, void *user_data){
	MF_VNRoutingTableCompute *handler = ((timerInfo_t *)user_data)->handler;
	handler->run_timer(t,user_data);
}

/**
 * Timer Task handler
 *
 * Rebuild forwarding table from current routing state
 * 
 */
void MF_VNRoutingTableCompute::run_timer(Timer *t, void *user_data){
	timerInfo_t *info = (timerInfo_t *)user_data;
	logger.log(MF_DEBUG, "MF_VNRoutingTableCompute:: Running timer for VN %u", info->guid);
	assert (info->timer == t);
	if(timerMap.get(info->guid) == timerMap.default_value()) return;
	MF_VNInfoContainer *container = _vnManager->getVNContainer(info->guid);
	if(container == NULL || !container->isInitialized()) {
		logger.log(MF_DEBUG, "MF_VNRoutingTableCompute:: VN %u doesn't exist anymore", info->guid);
		return;
	}
	MF_VNRoutingTable *routingTable = container->getVNRoutingTableConcurrent();
	//For virtual neighbor table
	if (routingTable->isVrtUpdateFlag() || routingTable->getVrtSkippedUpdates() >= MAX_VRT_SKIPPED_UPDATES) {
		routingTable->setVrtUpdateFlag(false);
		routingTable->setVrtSkippedUpdates(0);
		logger.log(MF_DEBUG, "virtualRT: Time to update virtual forward table");
		container->returnVNRoutingTable();
		computeForwardtable(info->guid);
		routingTable->printForwardingTable();
		routingTable->printASPTable();
	} else {
		routingTable->setVrtSkippedUpdates(routingTable->getVrtSkippedUpdates() + 1);
		container->returnVNRoutingTable();
	}
	info->timer->schedule_after_msec(VRT_UPDATE_PERIOD_MSECS);
}

void MF_VNRoutingTableCompute::addVNTimer(uint32_t vnGUID){
	logger.log(MF_DEBUG, "MF_VNRoutingTableCompute:: Adding timer for VN %u", vnGUID);
	if(timerMap.get(vnGUID) == timerMap.default_value()){
		timerInfo_t *info = new timerInfo_t();
		Timer *newTimer = new Timer(&callback, (void*)info);
		info->handler = this;
		info->timer = newTimer;
		info->guid = vnGUID;
		timerMap.set(vnGUID, info);
		info->timer->initialize(this);
		info->timer->schedule_after_msec(VRT_UPDATE_PERIOD_MSECS);
	} else {
		logger.log(MF_WARN, "MF_VNRoutingTableCompute:: Timer for VN %u already existing", vnGUID);
	}
}

void MF_VNRoutingTableCompute::removeVNTimer(uint32_t vnGUID){
	timerInfo_t *info = timerMap.get(vnGUID);
	if(info != timerMap.default_value()){
		info->timer->unschedule();
		timerMap.erase(vnGUID);
		delete info;
	}
}

void MF_VNRoutingTableCompute::computeForwardtable(uint32_t vnGUID) {
	logger.log(MF_DEBUG, "MF_VNRoutingTableCompute:: Recompute routing table for VN %u", vnGUID);

	uint32_t nodeMetric, vlinkCost;
	uint32_t virtual_address, true_address;
	uint32_t min_vlinkcost = MAX_WEIGHT;       // the maximum the uint32_t can support
	uint32_t virtual_dest_guid;
	uint32_t virtual_next_hop_guid = 0;
	uint32_t virtual_path_ip[MAX_PATH_LENGTH];
	int pl = 0;

	MF_VNInfoContainer *container = _vnManager->getVNContainer(vnGUID);

	virtualNSP_map * virtual_nsp_all = container->getVNNeighborTable()->getVirtualNSPmap();
	if(virtual_nsp_all->size() == 0) {
		return;
	}

	virtualtent_map *tentative_table = new virtualtent_map();

	uint32_t confirmed[MAX_PATH_LENGTH];
	int size_confirmed = 0;

	Vector<const virtualNeighbor_t*> nbrs_vec;
	container->getVNNeighborTable()->getVirtualNeighbors(nbrs_vec);

	//Dijsktra's Algorithm (Forward Search)
	//Initial setup of tentative entries for neighbors
	for (uint32_t i = 0; i < nbrs_vec.size(); ++i) {
		const virtualNeighbor_t *nbr = nbrs_vec[i];
		nodeMetric = nbr->nodeMetric;
		vlinkCost = nbr->vlinkCost;
		min_vlinkcost = vlinkCost;
		virtual_address = nbr->virtual_guid;
		true_address = nbr->true_guid;
		virtual_dest_guid = virtual_address;
		virtual_next_hop_guid = virtual_address;

		virtual_tentative_history_t *tent_entry = new virtual_tentative_history_t();

		tent_entry->vlinkcost = min_vlinkcost;
		tent_entry->nodemetric = nodeMetric;
		tent_entry->nexthopguid = virtual_next_hop_guid;
		tent_entry->virtualpath[0] = virtual_next_hop_guid;
		tent_entry->virtualpathlength = 1;

		tentative_table->set(virtual_dest_guid, tent_entry);
	}

	//TODO used ordered list instead of hash map and then just pull the first one everytime
	min_vlinkcost = MAX_WEIGHT;
	uint32_t wt;
	while (tentative_table->size() > 0) {
		for (virtualtent_map::const_iterator iter = tentative_table->begin(); iter != tentative_table->end(); ++iter) {
			wt = iter.value()->vlinkcost;
			if (min_vlinkcost > wt) {
				min_vlinkcost = wt;
				nodeMetric = iter.value()->nodemetric;
				virtual_dest_guid = iter.key();
				virtual_next_hop_guid = iter.value()->nexthopguid;
				pl = iter.value()->virtualpathlength;
				for (uint32_t m = 0; m < pl; ++m) {
					virtual_path_ip[m] = iter.value()->virtualpath[m];
				}
			}
		}
		virtualtent_map::iterator toRemove = tentative_table->find(virtual_dest_guid);
		if(toRemove != tentative_table->end()){
			delete toRemove.value();
			tentative_table->erase(virtual_dest_guid);
		}

		int skip1 = 0;
		int skip2 = 0;

		vnForwardingEntry_t *entry = new vnForwardingEntry_t();

		entry->vlinkcost = min_vlinkcost;
		entry->nodemetric = nodeMetric;
		entry->nexthopguid = virtual_next_hop_guid;
		entry->virtualpathlength = pl;
		//entry->epoch = ft_epoch;

		for (uint32_t i = 0; i < pl; ++i) {
			entry->virtualpath[i] = virtual_path_ip[i];
		}

		//TODO make copies and one big concurrent replacement, not concurrent changes all the time
		container->getVNRoutingTableConcurrent()->getVirtualForwardingTable()->set(virtual_dest_guid,entry);
		container->returnVNRoutingTable();
		//end TODO

		confirmed[size_confirmed] = virtual_dest_guid;
		size_confirmed++;
		virtualNSP_map::iterator it = virtual_nsp_all->find(virtual_dest_guid);
		if (it != virtual_nsp_all->end()) {
			int length = it.value()->len;
			for (uint32_t i = 0; i < length; ++i) {
				skip1 = 0;
				skip2 = 0;
				virtual_tentative_history_t *tent_entry = new virtual_tentative_history_t;
				tent_entry->vlinkcost = it.value()->vlinkCost_n[i] + min_vlinkcost;
				tent_entry->nodemetric = it.value()->nodeMetric;
				tent_entry->nexthopguid = virtual_next_hop_guid;
				uint32_t destination = it.value()->virtual_nbr_n[i];
				tent_entry->virtualpathlength = pl + 1;
				for (uint32_t m = 0; m < pl; ++m) {
					tent_entry->virtualpath[m + 1] = virtual_path_ip[m];
				}
				tent_entry->virtualpath[0] = destination;

				for (uint32_t j = 0; j < size_confirmed; ++j) {
					if (destination == confirmed[j]) {
						skip1 = 1;
					}
				}

				for (virtualtent_map::const_iterator iter0 = tentative_table->begin(); iter0 != tentative_table->end(); ++iter0) {
					if (destination == iter0.key()) {
						if (tent_entry->vlinkcost > iter0.value()->vlinkcost) {
							skip2 = 1;
						} else {
							delete tentative_table->find(destination).value();
							tentative_table->erase(destination);
							break;
						}
					}
				}

				if (skip1 == 0 && skip2 == 0 && destination != container->getMyVirtualGuid()) {
					tentative_table->set(destination, tent_entry);
				}
			}
		}
		min_vlinkcost = MAX_WEIGHT;
	}

	delete tentative_table;

	//TODO make copies and one big concuurent replacement, not concurrent changes all the time
	container->getVNRoutingTableConcurrent()->updateASPMetrics();
	container->returnVNRoutingTable();
	//end TODO
}


CLICK_ENDDECLS

EXPORT_ELEMENT(MF_VNRoutingTableCompute)
