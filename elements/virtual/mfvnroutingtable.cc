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
#include <vector>

#include "mffunctions.hh"
#include "mfvnroutingtable.hh"

/*
Reference:
http://www.orbit-lab.org/browser/mf/router/click/elements/gstar/mfroutingtable.cc?rev=0d66f570a8661ebb2e29238d220ace3e33c1f0bd

 */

CLICK_DECLS
MF_VNRoutingTable::MF_VNRoutingTable() : logger(MF_Logger::init()), vrt_update_flag(false), vrt_skipped_updates(0),
_virtualForwardingTable(), virtual_service_all(), virtual_asp_all(){
}

MF_VNRoutingTable::~MF_VNRoutingTable() {
}

void MF_VNRoutingTable::initialize(String &service_guid_file){
    parseServiceFile(service_guid_file);
}

void MF_VNRoutingTable::parseServiceFile(String &service_guid_file){
    FILE * file = fopen(service_guid_file.c_str(), "r");

    if(!file){
        logger.log(MF_CRITICAL, 
                "virtualRT: Could not open %s", service_guid_file.c_str());
        //continue without topology enforcement
        return;
    }
    logger.log(MF_INFO, 
            "virtualRT: Using topology file '%s'", service_guid_file.c_str());

    uint32_t service_guid, server_cnt, server_guid;

    bool done = false;

    //max size of a line in the topo file
    char line[TMP_LINE_SIZE];
    char *buf;
    int line_no = 1;
    int res; 
    while(!feof(file)){
        //skip over empty lines
        if((res = fscanf(file, "%a[ \t\v\f]\n", &buf)) == 1) {
            free(buf);
            logger.log(MF_TRACE, 
                    "virtualRT: skipping empty line in topo file at line# %u",
                    line_no);
            line_no++;
            continue;
        }
        //skip over comment lines
        if((res = fscanf(file, " #%a[^\n]\n", &buf)) == 1) {
            free(buf);
            logger.log(MF_TRACE, 
                    "virtualRT: skipping comment line in topo file at line# %u",
                    line_no);
            line_no++;
            continue;
        }
        // read 'service_guid server_cnt'
        if((res = fscanf(file, " %u %u", &service_guid, &server_cnt)) == EOF) {
            break;
        } else if (res != 2) {
            logger.log(MF_FATAL, 
                    "virtualRT: Error 1 while reading topo file at line %u",
                    line_no);
            exit(-1);

        }

        for(unsigned i = 0; i < server_cnt; i++){
            if(fscanf(file,"%d", &server_guid) != 1){
                logger.log(MF_FATAL, 
                        "virtualRT: Error 2 while reading topo file at line %u",
                        line_no);
                exit(-1);
            }
            server_guid_array[i] = server_guid;
            logger.log(MF_INFO, "virtualRT: Neighbor guid: %u", server_guid_array[i]);
        }
        insertServiceGuidMap(service_guid, server_cnt, server_guid_array); //Find true GUID
        //skip anything else on this line
        if(!fgets(line, TMP_LINE_SIZE, file)) {
            logger.log(MF_FATAL, 
                    "virtualRT: Error while reading topo file at line %u",
                    line_no);
            exit(-1);
        }
        line_no++;
    }
    printServiceTable();
}

/**
  * Request virtual forward table rebuild 
  *
  * Sets flag read by periodic timer task that recomputes the 
  * virtual forwarding table if the flag is set.
  */
void MF_VNRoutingTable::req_vrt_update(){
	vrt_update_flag = true;
}

int MF_VNRoutingTable::insertVirtualASP(VIRTUAL_NETWORK_ASP * virtual_network_asp_info) {
    int lsa_flag = 0;
    uint32_t virtual_src = virtual_network_asp_info->senderVirtual;
    logger.log(MF_DEBUG, "virtual_RT: insertVirtualASP: virtual header src: %u ", virtual_src);
    logger.log(MF_DEBUG, "virtual_RT: insertVirtualASP: type %u seq %u nodeMetric %u",
    		virtual_network_asp_info->type, virtual_network_asp_info->seq,
			virtual_network_asp_info->nodeMetric);

    vnASPMap::iterator iter = virtual_asp_all.find(virtual_src);
    //if present in map update the metric values 
    if(iter != virtual_asp_all.end()) {
        uint32_t seq_no_rcvd = virtual_network_asp_info->seq;
        uint32_t seq_no_prev = iter.value()->seq_no;
        logger.log(MF_DEBUG, "virtual_RT: insertVirtualASP: seq numbers: %u %u ", seq_no_rcvd, seq_no_prev);
        //check to see if the sequence no of the LSA received 
        //is greater than that of the LSA already saved 
        if (seq_no_rcvd > seq_no_prev) {
            iter.value()->seq_no = virtual_network_asp_info->seq;
            iter.value()->nodeMetric = virtual_network_asp_info->nodeMetric;
            lsa_flag = 1;
        }
    } else { //if not present in map insert
    	virtualASP * virtual_network_asp_info_copy = new virtualASP;
    	virtual_network_asp_info_copy->nodeMetric = virtual_network_asp_info->nodeMetric;
		virtual_network_asp_info_copy->seq_no = virtual_network_asp_info->seq;
    	virtual_asp_all.set(virtual_src, virtual_network_asp_info_copy);
        lsa_flag = 1;
    }

    return lsa_flag;
}

void MF_VNRoutingTable::updateASPMetrics() {
	for(vnASPMap::iterator asp_iter = virtual_asp_all.begin(); asp_iter != virtual_asp_all.end(); asp_iter++) {
		vnForwardingTable::iterator fwdmap_iter = _virtualForwardingTable.find(asp_iter.key());
		fwdmap_iter.value()->nodemetric = asp_iter.value()->nodeMetric;	//may not wrk -> have to set again
	}
}

//void MF_VNRoutingTable::insertVirtualForwardingTable(uint32_t neighb_virtual) {
//    virtualforward_t * temp = new virtualforward_t();
//    temp->nexthopguid = 0;
//    _virtualForwardingTable->set(neighb_virtual, temp);
//}

void MF_VNRoutingTable::insertServiceGuidMap(uint32_t service_guid, int server_cnt, uint32_t server_guid_array[]) {
	vnServiceList_t * temp = new vnServiceList_t();
    temp->size = server_cnt;
    for(int i = 0; i < server_cnt; i++) {
	temp->virtual_guid_servers[i] = server_guid_array[i];
    }	
    virtual_service_all.set(service_guid, temp);
}

uint32_t MF_VNRoutingTable::getVirtualNextHopGUID(uint32_t virtualDestGUID)
{
    uint32_t virtualNextHopGUID = -1;
    vnForwardingTable::iterator it = _virtualForwardingTable.find(virtualDestGUID);
    if(it != _virtualForwardingTable.end()) {
        virtualNextHopGUID = it.value()->nexthopguid;
    } 

    return virtualNextHopGUID;
}

uint32_t MF_VNRoutingTable::getVlinkCost(uint32_t virtualDestGUID)
{
    uint32_t vlinkCost = -1;
    vnForwardingTable::iterator it = _virtualForwardingTable.find(virtualDestGUID);
    if(it != _virtualForwardingTable.end()) {
        vlinkCost = it.value()->vlinkcost;
    }

    return vlinkCost;
}

vnForwardingTable* MF_VNRoutingTable::getVirtualForwardingTable() {
    return &_virtualForwardingTable;
}

void MF_VNRoutingTable::printForwardingTable(){
  logger.log(MF_DEBUG, "virtual_rtg_tbl: ######### Virtual Forward Table #########");
  for (vnForwardingTable::const_iterator iter1 = _virtualForwardingTable.begin(); iter1 != _virtualForwardingTable.end(); ++iter1) {
    logger.log(MF_DEBUG, "virtual_rtg_tbl: virtual FT entry: virtual dst=%d, virtual nexthop=%d,virtual pathlength=%d, virtual vlink=%u, nodemetric=%u",
      iter1.key(), iter1.value()->nexthopguid, iter1.value()->virtualpathlength, iter1.value()->vlinkcost, iter1.value()->nodemetric);
  }
}

void MF_VNRoutingTable::printASPTable() {
    logger.log(MF_INFO, "virtual_rtg_tbl:---------------------Virtual ASP Map----------------------------");
    for(vnASPMap::const_iterator iter = virtual_asp_all.begin(); iter != virtual_asp_all.end(); ++iter) {
        uint32_t guid = iter.key();
        virtualASP_t *node = iter.value();
        logger.log(MF_INFO, "\t \t guid: %u nodemetric: %u seq_no: %u",guid, node->nodeMetric, node->seq_no);	
    }
}

void MF_VNRoutingTable::printServiceTable() {
    logger.log(MF_INFO, "\n \nvirtualRT:-----------------Service Map------------------");
    for (vnServiceTable::const_iterator iter = virtual_service_all.begin(); iter != virtual_service_all.end(); ++iter){
        uint32_t guid = iter.key();
	int size = iter.value()->size;
	for(int i = 0; i < size; i++) {
            logger.log(MF_INFO, "\t \t %u %u",guid, iter.value()->virtual_guid_servers[i]);
	}
    }
}

CLICK_ENDDECLS

ELEMENT_PROVIDES(MF_VNRoutingTable)
