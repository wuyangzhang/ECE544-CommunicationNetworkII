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
* MF_VirtualDataForwarding.cc
* Forward virtual data packet to next virtual hop
* Forwarding can be done to a specified destination node or using service anycast  
* Implements algorithms for Application Specific Routing with service anycast
* Refers to MF_VirtualRoutingTable.cc for routing information

*      Author: Aishwarya Babu
************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "mfvndataforwarding.hh"
#include "mfvnroutingtable.hh"
#include "mfvnmanager.hh"
#include "mfvninfocontainer.hh"

CLICK_DECLS

MF_VNDataForwarding::MF_VNDataForwarding():
    logger(MF_Logger::init()){
    algorithm_indicator = ALGO_1;
    }

MF_VNDataForwarding::~MF_VNDataForwarding(){

}

//TODO: what is MY_VIRTUAL_NETWORK_GUID?
int MF_VNDataForwarding::configure(Vector<String> &conf, ErrorHandler *errh){
    if (cp_va_kparse(conf, this, errh,
				"VN_MANAGER", cpkP+cpkM, cpElement, &_vnManager,
                "CHUNK_MANAGER", cpkP+cpkM, cpElement, &_chunkManager,
                cpEnd) < 0) {
        return -1;
    }

    return 0; 
}

/*!
  Incoming packet sent for forwarding
*/
void MF_VNDataForwarding::push (int port, Packet * pkt){
    if (port == 0) {
    	forwardVirtualData(pkt);
    } else {
        logger.log(MF_FATAL, 
                "virtual_data_forwarding: Packet received on unsupported port");
        exit(-1);
    }
}

/*!
 * Packet to be forwarded to next hop based on service anycast algorithm (if destination GUID is service GUID)
 * or based on destination guid 
 * Re populates the routing header (does not make new chunks to forward) 
*/
void MF_VNDataForwarding::forwardVirtualData(Packet *pkt){
    logger.log(MF_DEBUG, "VNDataForwarding: received virtual data packet");

    chk_internal_trans_t* chk_trans = (chk_internal_trans_t*)pkt->data();
    uint32_t sid = chk_trans->sid;
    uint32_t chunk_id = chk_trans->chunk_id;
    Chunk *chunk = _chunkManager->get(chunk_id, ChunkStatus::ST_INITIALIZED);
    if (!chunk) {
        logger.log(MF_ERROR, "VNDataForwarding: to_file: Got a invalid chunk from ChunkManager");
        return;
    }
    VirtualExtHdr vextheader(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);
    uint32_t vguid_dest = vextheader.getVirtualDestinationGUID().to_int();
    logger.log(MF_DEBUG, "VNDataForwarding: vguid_Dest is: %u", vguid_dest);

    MF_VNInfoContainer *container =
    		_vnManager->getVNContainer(vextheader.getVirtualNetworkGUID().to_int());

    if(container == NULL) return;

    vnServiceTable * virtual_service_all;
    vnForwardingTable * virtualForwardingTable;
    virtual_service_all = container->getVNRoutingTable()->getVirtualServiceMap();


//Change this to compare the SID to SID_VIRTUAL | SID_EXTHEADER | SID_ANYCAST
    uint32_t guid_nexthop;
    vnServiceTable::iterator service_iter = virtual_service_all->find(vguid_dest);
    if(service_iter != virtual_service_all->end()) {
		logger.log(MF_DEBUG, "VNDataForwarding: service guid: %u", vguid_dest);

		uint32_t vguid_dest_temp;
		switch(algorithm_indicator) {
			 case ALGO_1:
			logger.log(MF_DEBUG,"VNDataForwarding: Algorithm : %u", ALGO_1);
			vguid_dest_temp = findBestDestination(container, vguid_dest);
			break;
			 case ALGO_2:
			logger.log(MF_DEBUG,"VNDataForwarding: Algorithm : %u", ALGO_2);
			vguid_dest_temp = findBestDestination_algo2(container, vguid_dest);
			break;
		}

		virtualForwardingTable =
				container->getVNRoutingTableConcurrent()->getVirtualForwardingTable();

		vnForwardingTable::iterator iter = virtualForwardingTable->find(vguid_dest_temp);
		if(iter != virtualForwardingTable->end()){
			uint32_t vguid_nexthop = iter.value()->nexthopguid;
        	guid_nexthop = container->getVNGUIDMap()->getTrueGUID(vguid_nexthop);
		} else {
			logger.log(MF_WARN,"VNDataForwarding: No forwarding entry for GUID : %u", vguid_dest_temp);
			//TODO decide what to do with not resolvable chunks
			_chunkManager->removeData(chunk);
			pkt->kill();
			container->returnVNRoutingTable();
			return;
		}
    } else {
    	//TODO: should it be discarded instead?
    	logger.log(MF_DEBUG,"VNDataForwarding: No service entry for guid: %u. "
    			"Using normal GUID routing", vguid_dest);
    	virtualForwardingTable = container->getVNRoutingTableConcurrent()->getVirtualForwardingTable();
    	vnForwardingTable::iterator iter = virtualForwardingTable->find(vguid_dest);
        if(iter != virtualForwardingTable->end()){
			uint32_t vguid_nexthop = iter.value()->nexthopguid;
	        guid_nexthop = container->getVNGUIDMap()->getTrueGUID(vguid_nexthop);
		} else {
			logger.log(MF_WARN,"VNDataForwarding: No forwarding entry for GUID : %u", vguid_dest);
			//TODO decide what to do with not resolvable chunks
			_chunkManager->removeData(chunk);
			pkt->kill();
			container->returnVNRoutingTable();
			return;
		}
    }

    container->returnVNRoutingTable();

    RoutingHeader * rheader = chunk->routingHeader();
    uint32_t guid_dest_old = rheader->getDstGUID().to_int();
    logger.log(MF_DEBUG, "VNDataForwarding: handleVirtualDATA: dest guid old : %u ", guid_dest_old);
    rheader->setSrcNA(0);
    rheader->setDstGUID(guid_nexthop);
    rheader->setDstNA(0);
    uint32_t guid_dest_new = rheader->getDstGUID().to_int();
    logger.log(MF_DEBUG, "VNDataForwarding: handleVirtualDATA: dest guid new : %u ", guid_dest_new);

//Adding to chunk manager's destination GUID map after changing the destination GUID in rheader
    _chunkManager->addToDstGUIDTable(chunk_id);
    _chunkManager->removeFromDstGUIDTable(chunk_id, guid_dest_old);

//TODO: Instead of setting here .. might need a cleaner solution 
    chk_trans->upper_protocol = TRANSPORT; 
    output(0).push(pkt);	
}


/*
    Algorithm 1:
        1. For all node metrics less than the threshold choose the shorter vlink cost destination 
        2. If all node metrics > threshold choose the shorter vlink cost destination 
*/
uint32_t MF_VNDataForwarding::findBestDestination(MF_VNInfoContainer *container, uint32_t service_guid_dest) {
	vnForwardingTable * virtualForwardingTable;
    vnServiceTable * virtual_service_all;

    virtual_service_all = container->getVNRoutingTable()->getVirtualServiceMap();

    vnServiceTable::iterator service_iter = virtual_service_all->find(service_guid_dest);
    uint32_t size_temp = service_iter.value()->size;
    logger.log(MF_DEBUG, "VNDataForwarding: number of potential destination for guid %u: %u ",
    		service_guid_dest, size_temp);

    uint32_t minLinkCost, minLinkGUID = 0;
    double minNodeMetric;
    double printNodeMetric;

    minNodeMetric = 1;

    virtualForwardingTable = container->getVNRoutingTableConcurrent()->getVirtualForwardingTable();
    for(int i = 0; i < size_temp; i++) {
        uint32_t temp_guid = service_iter.value()->virtual_guid_servers[i];
        vnForwardingTable::iterator fwdmap_iter = virtualForwardingTable->find(temp_guid);
        if(fwdmap_iter != virtualForwardingTable->end()) {
			double tempNodeMetric = (double)fwdmap_iter.value()->nodemetric/ UINT_MAX;
			logger.log(MF_DEBUG, "VNDataForwarding: App value for guid %u: %f",
					temp_guid, tempNodeMetric);
			if(tempNodeMetric < minNodeMetric) {
				minNodeMetric = tempNodeMetric;
			}
        } else {
        	logger.log(MF_DEBUG, "VNDataForwarding: No forwarding entry for guid %u ", temp_guid);
        	continue;
        }
    }

    if(minNodeMetric > NODE_METRIC_THRESHOLD) {
    	logger.log(MF_DEBUG, "VNDataForwarding: No nodes have App metric lower than threshold %f. "
    			"Deciding based on best path", NODE_METRIC_THRESHOLD);
        minLinkCost = MAX_WEIGHT;
        for(int i = 0; i < size_temp; i++){
            uint32_t temp_guid = service_iter.value()->virtual_guid_servers[i];
            vnForwardingTable::iterator fwdmap_iter = virtualForwardingTable->find(temp_guid);
            if(fwdmap_iter != virtualForwardingTable->end()) {
				double tempNodeMetric = (double)fwdmap_iter.value()->nodemetric/ UINT_MAX;
				uint32_t tempLinkCost = fwdmap_iter.value()->vlinkcost;
				logger.log(MF_DEBUG, "VNDataForwarding: App value and link metric for guid %u: %f,%u",
									temp_guid, tempNodeMetric,tempLinkCost);
				if(tempLinkCost < minLinkCost) {
					minLinkCost = tempLinkCost;
					minLinkGUID = temp_guid;
					printNodeMetric = tempNodeMetric;
				}
            } else {
            	logger.log(MF_DEBUG, "VNDataForwarding: No forwarding entry for guid %u ", temp_guid);
            	continue;
            }
        }

    } else {
    	logger.log(MF_DEBUG, "VNDataForwarding: Deciding based on App metric with threshold %f",
    			NODE_METRIC_THRESHOLD);
        minLinkCost = MAX_WEIGHT;
        for(int i = 0; i < size_temp; i++){
            uint32_t temp_guid = service_iter.value()->virtual_guid_servers[i];
            vnForwardingTable::iterator fwdmap_iter = virtualForwardingTable->find(temp_guid);
            if(fwdmap_iter != virtualForwardingTable->end()) {
				double tempNodeMetric = (double)fwdmap_iter.value()->nodemetric/ UINT_MAX;
				uint32_t tempLinkCost = fwdmap_iter.value()->vlinkcost;
				logger.log(MF_DEBUG, "VNDataForwarding: App value and link metric for guid %u: %f,%u",
									temp_guid, tempNodeMetric,tempLinkCost);
				if(tempNodeMetric < NODE_METRIC_THRESHOLD && tempLinkCost < minLinkCost){
					minLinkCost = tempLinkCost;
					minLinkGUID = temp_guid;
					printNodeMetric = tempNodeMetric;
				}
            } else {
            	logger.log(MF_DEBUG, "VNDataForwarding: No forwarding entry for guid %u ", temp_guid);
            	continue;
            }
        }
    }
    container->returnVNRoutingTable();
    logger.log(MF_DEBUG, "VNDataForwarding: min guid is %u with nodemetric: %f min link cost %u", minLinkGUID, printNodeMetric, minLinkCost);
    return minLinkGUID;
}

/*
 * Algo 2: Combination of link metric: estimated file transfer time and node metric: waiting time of cloud site 
 * Estimated file transfer time : aggregated SETT scaled by the size of the file being transfered
*/
uint32_t MF_VNDataForwarding::findBestDestination_algo2(MF_VNInfoContainer *container, uint32_t service_guid_dest) {
	vnForwardingTable * virtualForwardingTable;
    vnServiceTable * virtual_service_all;
    virtual_service_all = container->getVNRoutingTable()->getVirtualServiceMap();

    vnServiceTable::iterator service_iter = virtual_service_all->find(service_guid_dest);
    uint32_t size_temp = service_iter.value()->size;
    logger.log(MF_DEBUG, "VNDataForwarding: number of best dest guids: %u ", size_temp);

    uint32_t minCost, minGUID;

    minCost = MAX_TIME_VALUE_MSECS;

    virtualForwardingTable = container->getVNRoutingTableConcurrent()->getVirtualForwardingTable();
    for(int i = 0; i < size_temp; i++) {
        uint32_t temp_guid = service_iter.value()->virtual_guid_servers[i];
        vnForwardingTable::iterator fwdmap_iter = virtualForwardingTable->find(temp_guid);
        
        //TODO: Algorithm specifics need to be fixed 
        uint32_t tempCost = fwdmap_iter.value()->nodemetric + fwdmap_iter.value()->vlinkcost;
// uint32_t tempCost = fwdmap_iter.value()->nodemetric / MAX_VALUE_FTT + fwdmap_iter.value()->vlinkcost * file_size;
        if(tempCost < minCost) {
            minCost = tempCost;
        }
    }
    container->returnVNRoutingTable();
}

CLICK_ENDDECLS

EXPORT_ELEMENT(MF_VNDataForwarding)
