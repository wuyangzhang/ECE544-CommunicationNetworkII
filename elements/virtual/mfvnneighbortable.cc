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

#include "mfvnneighbortable.hh"
#include "mfvnguidmap.hh"
#define TMP_LINE_SIZE 1000

/*
Reference:
http://www.orbit-lab.org/browser/mf/router/click/elements/gstar/mfroutingtable.cc?rev=0d66f570a8661ebb2e29238d220ace3e33c1f0bd

 */

CLICK_DECLS
MF_VNNeighborTable::MF_VNNeighborTable(uint32_t my_guid,uint32_t _virtualNetworkGUID,uint32_t my_virtual_guid) : logger(MF_Logger::init()), vnbrt_update_flag(false), vnbrt_skipped_updates(0) {
	this->my_guid = my_guid;
	this->_virtualNetworkGUID = _virtualNetworkGUID;
	this->my_virtual_guid = my_virtual_guid;
	this->vguid_map = NULL;
    _virtualNeighborTable = new virtualNeighbor_map();
    virtual_nsp_all = new virtualNSP_map();
}

MF_VNNeighborTable::~MF_VNNeighborTable() {
}

void MF_VNNeighborTable::initialize(String &file,MF_VNGUIDmap *map){
	this->vguid_map = map;
	algorithm_indicator = vguid_map->getAlgorithmIndicator();
    parseTopologyFile(file);
}

void MF_VNNeighborTable::parseTopologyFile(String topo_file){
    //full path for topology file

    FILE * file = fopen(topo_file.c_str(), "r");

    if(!file){
        logger.log(MF_CRITICAL, 
                "virtual_nbr_tbl: Could not open %s", topo_file.c_str());
        //continue without topology enforcement
        return;
    }
    logger.log(MF_INFO, 
            "virtual_nbr_tbl: Using topology file '%s'", topo_file.c_str());

    uint32_t vguid, nbr_cnt, nbr_guid;

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
                    "virtual_nbr_tbl: skipping empty line in topo file at line# %u",
                    line_no);
            line_no++;
            continue;
        }
        //skip over comment lines
        if((res = fscanf(file, " #%a[^\n]\n", &buf)) == 1) {
            free(buf);
            logger.log(MF_TRACE, 
                    "virtual_nbr_tbl: skipping comment line in topo file at line# %u",
                    line_no);
            line_no++;
            continue;
        }
        // read 'guid nbr_cnt'
        if((res = fscanf(file, " %u %u", &vguid, &nbr_cnt)) == EOF) {
            break;
        } else if (res != 2) {
            logger.log(MF_FATAL, 
                    "virtual_nbr_tbl: Error 1 while reading topo file at line %u",
                    line_no);
            exit(-1);

        }

        if (vguid == my_virtual_guid) {
            num_neighbors = nbr_cnt;
            logger.log(MF_INFO, 
                    "virtual_nbr_tbl: Total neighbors: %u", num_neighbors);
        }
        for(unsigned i = 0; i < nbr_cnt; i++){
            if(fscanf(file,"%d", &nbr_guid) != 1){
                logger.log(MF_FATAL, 
                        "virtual_nbr_tbl: Error 2 while reading topo file at line %u",
                        line_no);
                exit(-1);
            }
            if(vguid == my_virtual_guid){
                vguid_array[i] = nbr_guid;
                logger.log(MF_INFO, "virtual_nbr_tbl: Neighbor guid: %u %u", vguid_array[i], vguid_map->getTrueGUID(nbr_guid));
                insertVirtualNeighborTable(vguid_array[i], vguid_map->getTrueGUID(nbr_guid));
            }
        }
        //skip anything else on this line
        if(!fgets(line, TMP_LINE_SIZE, file)) {
            logger.log(MF_FATAL, 
                    "virtual_nbr_tbl: Error while reading topo file at line %u",line_no);
            exit(-1);
        }
        line_no++;
    }
} 

void MF_VNNeighborTable::print() {
    logger.log(MF_INFO, "virtual_nbr_tbl:--------------------- Virtual Neighbor table:-------------------");
    for (virtualNeighbor_map::const_iterator iter = _virtualNeighborTable->begin(); iter != _virtualNeighborTable->end(); ++iter){
        uint32_t guid = iter.key();
        virtualNeighbor_t *nbr = iter.value();
//	uint32_t agg_sett = getAggregatedSETT(guid);
        //TODO: why not stored aggregated?
        logger.log(MF_INFO, "\t \t %u %u \t %u \t agg sett: %u",guid, nbr->true_guid, nbr->vlinkCost);//, agg_sett);
    }
}

void MF_VNNeighborTable::printNSPmap() {
    logger.log(MF_INFO, "virtual_nbr_tbl:---------------------Virtual NSP Map----------------------------");
    for(virtualNSP_map::const_iterator iter = virtual_nsp_all->begin(); iter != virtual_nsp_all->end(); ++iter) {
        uint32_t guid = iter.key();
        virtualNSP_t *node = iter.value();
        logger.log(MF_INFO, "\t \t guid: %u nodemetric: %u seq_no: %u num_neigh: %u",guid, node->nodeMetric, node->seq_no, node->len);	
	for(int i = 0; i < node->len; i++)
	{
	    logger.log(MF_INFO, "\t \t \t \t \t virtual nbr: %u vlink cost %u", node->virtual_nbr_n[i], node->vlinkCost_n[i]);	
	}
    }
}

int MF_VNNeighborTable::insertVirtualNSP(Chunk * chunk) {

    int update = 0;
    int lsa_flag = 0;
    RoutingHeader* rheader = chunk->routingHeader();
    uint32_t src = rheader->getSrcGUID().to_int();
    uint32_t dest = rheader->getDstGUID().to_int();
    logger.log(MF_INFO, "virtual_lsa_handler: rheader src: %u", src);
    logger.log(MF_INFO, "virtual_lsa_handler: rheader dest: %u", dest);

    VirtualExtHdr * vextheader = new VirtualExtHdr(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE);

    uint32_t virtual_net = vextheader->getVirtualNetworkGUID().to_int();  //Line of seg fault 
    //uint32_t virtual_src = vextheader->getVirtualSourceGUID().to_int();
    uint32_t virtual_dest = vextheader->getVirtualDestinationGUID().to_int();
    logger.log(MF_INFO, "virtual_lsa_handler: virtual header network: %u ", virtual_net);
    logger.log(MF_INFO, "virtual_lsa_handler: virtual header dest: %u ", virtual_dest);

   //Used only for printing
    delete vextheader;

    VIRTUAL_NETWORK_SA * virtual_network_sa_info;
    virtual_network_sa_info = (VIRTUAL_NETWORK_SA*)(chunk->allPkts()->at(0)->data() + HOP_DATA_PKT_SIZE + ROUTING_HEADER_SIZE + EXTHDR_VIRTUAL_MAX_SIZE);
    uint32_t virtual_src = virtual_network_sa_info->senderVirtual;

    virtualNSP_map::iterator iter = virtual_nsp_all->find(virtual_src);
    logger.log(MF_INFO, "virtual_lsa_handler: virtual header src: %u ", virtual_src);
    //if present in map update the metric values 
    if(iter != virtual_nsp_all->end()) {
        uint32_t seq_no_rcvd = virtual_network_sa_info->seq;
        uint32_t seq_no_prev = iter.value()->seq_no;
		logger.log(MF_DEBUG, "virtual_lsa_handler: seq numbers: %u %u ", seq_no_rcvd, seq_no_prev);
        //check to see if the sequence no of the LSA received 
        //is greater than that of the LSA already saved 
        if (seq_no_rcvd > seq_no_prev) {						
            delete [] iter.value()->virtual_nbr_n;
            delete [] iter.value()->vlinkCost_n;
            //delete iter.value()->expiry;
            delete iter.value();
            virtual_nsp_all->erase(iter); 
            update = 1;
        }
    } else { //if not present in map insert 
        update = 1;
    }

    if(update == 1) {
        int size = virtual_network_sa_info->size; 

	virtualNSP_t * tmp = new virtualNSP_t();
        assert(tmp);

        tmp->len = size;
        tmp->seq_no = virtual_network_sa_info->seq;
        tmp->nodeMetric = virtual_network_sa_info->nodeMetric;

	tmp->virtual_nbr_n = new uint32_t[size];
        assert(tmp->virtual_nbr_n);  
        uint32_t * virtual_nbr_n_addr = (uint32_t *) (virtual_network_sa_info + 1); //figure out why 1 

        for (uint32_t i = 0; i < size; ++i) {
            tmp->virtual_nbr_n[i] = *virtual_nbr_n_addr;
	    virtual_nbr_n_addr++; 
        }

	tmp->vlinkCost_n = new uint32_t[size];
        assert(tmp->vlinkCost_n);  
        uint32_t * vlinkCost_n_addr = (uint32_t *) (virtual_nbr_n_addr); 

        for (uint32_t i = 0; i < size; ++i) {
            tmp->vlinkCost_n[i] = *vlinkCost_n_addr;
	    vlinkCost_n_addr++; 
        }

        virtual_nsp_all->set(virtual_src, tmp);     
	lsa_flag = 1;
    }
	return lsa_flag;
}

void MF_VNNeighborTable::insertVirtualNeighborTable(uint32_t virtualNbr, uint32_t trueNbr) {

    virtualNeighbor_map::iterator it = _virtualNeighborTable->find(virtualNbr);

    if(it != _virtualNeighborTable->end()) {
        it.value()->virtual_guid = virtualNbr;
        it.value()->true_guid = trueNbr;
        _virtualNeighborTable->set(virtualNbr, it.value());
    } else {
        virtualNeighbor_t * temp = new virtualNeighbor_t();
        temp->virtual_guid = virtualNbr;
        temp->true_guid = trueNbr;
        temp->nodeMetric = 0;
        temp->vlinkCost = MAX_VLINK_COST; //MAX VALUE
        _virtualNeighborTable->set(virtualNbr, temp);
    }

    logger.log(MF_INFO, "virtual_nbr_tbl: Inserting virtual neighbor %u", virtualNbr);
}

void MF_VNNeighborTable::req_vnbrt_update() {
    vnbrt_update_flag = true;
}

//Getters

uint32_t MF_VNNeighborTable::getTrueGUID(uint32_t virtualGUID) {

    uint32_t trueGUID = -1;
    virtualNeighbor_t temp;
    virtualNeighbor_map::iterator it = _virtualNeighborTable->find(virtualGUID);
    if(it != _virtualNeighborTable->end()) {
        temp = *(it.value());
        trueGUID = temp.true_guid;
    }
    //Iterator through entries of hash table .. need to access the value->trueGUID

    return trueGUID;
}

/* Populates provided vector with current virtual neighbors' information */

void MF_VNNeighborTable::getVirtualNeighbors(Vector<const virtualNeighbor_t*> &nbrs_v){
    logger.log(MF_INFO, "virtual_nbr_tbl: Tryin to push into vector of neighbors");
    for (virtualNeighbor_map::const_iterator iter = _virtualNeighborTable->begin(); iter != _virtualNeighborTable->end(); ++iter) {
        nbrs_v.push_back(iter.value());
    }
}

virtualNeighbor_map* MF_VNNeighborTable::getVirtualNeighborMap() {
    return _virtualNeighborTable; 
}

virtualNSP_map* MF_VNNeighborTable::getVirtualNSPmap() {
    return virtual_nsp_all;
}

CLICK_ENDDECLS

ELEMENT_PROVIDES(MF_VNNeighborTable)
