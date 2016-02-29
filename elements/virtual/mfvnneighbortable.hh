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

#ifndef MF_VN_NEIGHBOR_TABLE_HH
#define MF_VN_NEIGHBOR_TABLE_HH

/* virtual forwarding table recompute interval */
#define VNBRT_UPDATE_PERIOD_MSECS 1000

/* force recompute of FT after max skips */
#define MAX_VNBRT_SKIPPED_UPDATES 2
#define MAX_VLINK_COST 10000

#include <click/hashtable.hh>
#include "mflogger.hh"
//#include <queue>
//#include <vector>
#include <pthread.h>
#include "mfroutingtable.hh"
#include "mfvnguidmap.hh"
#include "mfchunkmanager.hh"
#include "mfvirtualextheader.hh"
#include "mfvirtualctrlmsg.hh"
#include "mfvnguidmap.hh"
/*
   Virtual->True Map:      VM GUID | TRUE GUID (Only the virtual neighbors of the VM node)
   If for a specific destination VM GUID the virtual -> true map doesn't have an entry the node queries the gnrs. 

 */

CLICK_DECLS

typedef struct virtualNeighbor {
	uint32_t virtual_guid;
	uint32_t true_guid;
	uint32_t nodeMetric;
	uint32_t vlinkCost;
	Timer * expiry;
	uint32_t prev_seq_no;
}virtualNeighbor_t;

typedef struct virtualNSP {
	int seq_no;
	int len;
	uint32_t nodeMetric;
	uint32_t *virtual_nbr_n;
	uint32_t *vlinkCost_n;
}virtualNSP_t;

typedef HashTable<uint32_t, virtualNeighbor_t*> virtualNeighbor_map;
typedef HashTable<uint32_t, virtualNSP_t*> virtualNSP_map;

class MF_VNNeighborTable {

public:
	MF_VNNeighborTable(uint32_t,uint32_t,uint32_t);
	~MF_VNNeighborTable();

	void initialize(String&,MF_VNGUIDmap *);

	void parseTopologyFile(String); // needs to be used instead of parseFile once GNRS can be queried

	//Display neighbor table
	void print();
	void printNSPmap();

	int insertVirtualNSP(Chunk * chunk);

	void insertVirtualNeighborTable(uint32_t virtualNbr, uint32_t trueNbr); //needs to query GNRS later

	void req_vnbrt_update();

	uint32_t getTrueGUID(uint32_t);
	void getVirtualNeighbors(Vector<const virtualNeighbor_t*> &nbrs_v);
	virtualNeighbor_map* getVirtualNeighborMap();
	//		int getVirtualNSPmap(virtualNSP_map& virtual_nsp_all_1);
	virtualNSP_map* getVirtualNSPmap();

	uint32_t getFtEpoch() const {
		return ft_epoch;
	}

	void setFtEpoch(uint32_t ftEpoch) {
		ft_epoch = ftEpoch;
	}

	uint32_t getNumNeighbors() const {
		return num_neighbors;
	}

	void setNumNeighbors(uint32_t numNeighbors) {
		num_neighbors = numNeighbors;
	}

	unsigned getVnbrtSkippedUpdates() const {
		return vnbrt_skipped_updates;
	}

	void setVnbrtSkippedUpdates(unsigned vnbrtSkippedUpdates) {
		vnbrt_skipped_updates = vnbrtSkippedUpdates;
	}

	bool isVnbrtUpdateFlag() const {
		return vnbrt_update_flag;
	}

	void setVnbrtUpdateFlag(bool vnbrtUpdateFlag) {
		vnbrt_update_flag = vnbrtUpdateFlag;
	}

	int getAlgorithmIndicator() const {
		return algorithm_indicator;
	}

	void setAlgorithmIndicator(int algorithmIndicator) {
		algorithm_indicator = algorithmIndicator;
	}

private:
	MF_VNGUIDmap * vguid_map;

	virtualNeighbor_map * _virtualNeighborTable;

	virtualNSP_map * virtual_nsp_all;

	uint32_t ft_epoch;

	uint32_t _virtualNetworkGUID;
	uint32_t my_virtual_guid;
	uint32_t my_guid;

	int algorithm_indicator;

	uint32_t num_neighbors;
	uint32_t vguid_array[50];

	bool vnbrt_update_flag;
	unsigned vnbrt_skipped_updates;

	MF_Logger logger;
};

CLICK_ENDDECLS
#endif
