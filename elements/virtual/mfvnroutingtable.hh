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

#ifndef MF_VN_ROUTING_TABLE_HH
#define MF_VN_ROUTING_TABLE_HH

/* virtual forwarding table recompute interval */
#define VRT_UPDATE_PERIOD_MSECS 1000

/* force recompute of FT after max skips */
#define MAX_VRT_SKIPPED_UPDATES 2

/* For file parsing */
#define TMP_LINE_SIZE 1000


#include <click/hashtable.hh>
#include <pthread.h>

#include "mfvnpriorityqueue.hh"
#include "mf.hh"
#include "mflogger.hh"
#include "mfvnroutingtable.hh"
#include "mfvirtualctrlmsg.hh"

/*
   Virtual->True Map:      VM GUID | TRUE GUID (Only the virtual neighbors of the VM node)
   If for a specific destination VM GUID the virtual -> true map doesn't have an entry the node queries the gnrs. 

   Virtual Routing table:  VM GUID | ASR Metric | VM GUID Next hop
 */

CLICK_DECLS

typedef struct vnForwardingEntry {
	uint32_t vlinkcost;
	uint32_t nodemetric;
	uint32_t nexthopguid;
	uint32_t virtualpath[MAX_PATH_LENGTH];
	int virtualpathlength;
	uint32_t epoch;
}vnForwardingEntry_t;
//TODO remove
typedef vnForwardingEntry_t virtual_tentative_history_t;

typedef struct vnServiceList {
	uint32_t size;
	uint32_t virtual_guid_servers[MAX_PATH_LENGTH];
}vnServiceList_t;

typedef struct virtualASP {
    int seq_no;
    uint32_t nodeMetric;
}virtualASP_t;

typedef HashTable<uint32_t, vnForwardingEntry_t*> vnForwardingTable;
//TODO remove
typedef vnForwardingTable virtualtent_map;
typedef HashTable<uint32_t, MF_VNPriorityQueue*> vnRoutingTable; //When using queue: currently using forward_map for simplicity
typedef HashTable<uint32_t, vnServiceList_t*> vnServiceTable;
typedef HashTable<uint32_t, virtualASP*> vnASPMap;

class MF_VNRoutingTable {

	public:
		MF_VNRoutingTable();
		~MF_VNRoutingTable(); 

		const char* class_name() const        { return "MF_VirtualRoutingTable"; }

		void initialize(String &service_guid_file);

		void parseTopologyFile();
		void parseServiceFile(String &service_guid_file);

		//Update flag to recompute virtual routing table using djikstra at the advent of a new virtual network state packet
		void req_vrt_update(); 

		//Needs two types : insert Metric + hop & insert new entry into virtual map
		//return types = int for some flag indication 
		int insertVirtualASP(VIRTUAL_NETWORK_ASP * virtual_network_asp_info);
		void insertVirtualForwardingTable(uint32_t neighb_virtual);
		void insertServiceGuidMap(uint32_t service_guid, int server_cnt, uint32_t server_guid_array[]);
		void insertVirtualRoutingTable(uint32_t neighb_virtual);
		void insertReverseGUIDTable(uint32_t true_GUID, uint32_t virtual_GUID);

		//Setters
		void setSelfVirtualGUID(uint32_t selfVirtualGUID);
		void setVirtualNetworkGUID(uint32_t virtualNetworkGUID);

		//Getters 
		//		uint32_t getTrueGUID(uint32_t);
		uint32_t getVirtualNextHopGUID(uint32_t);
		uint32_t getVlinkCost(uint32_t);
//		uint32_t getAggregatedSETT(uint32_t);
		
		
		vnForwardingTable* getVirtualForwardingTable();
		vnServiceTable* getVirtualServiceMap() {return &virtual_service_all; }

		//Print tables
		void printAll();
        void printForwardingTable();
		void printASPTable();
		void printServiceTable();

		unsigned getVrtSkippedUpdates() const {
			return vrt_skipped_updates;
		}

		void setVrtSkippedUpdates(unsigned vrtSkippedUpdates) {
			vrt_skipped_updates = vrtSkippedUpdates;
		}

		bool isVrtUpdateFlag() const {
			return vrt_update_flag;
		}

		void setVrtUpdateFlag(bool vrtUpdateFlag) {
			vrt_update_flag = vrtUpdateFlag;
		}

		void updateASPMetrics();

	private:

		vnForwardingTable _virtualForwardingTable;
		vnRoutingTable _virtualRoutingTable;
		vnServiceTable virtual_service_all;
		vnASPMap virtual_asp_all;

		uint32_t num_neighbors;
		uint32_t server_guid_array[MAX_PATH_LENGTH];

		bool vrt_update_flag;
		unsigned vrt_skipped_updates;

		MF_Logger logger;
};

CLICK_ENDDECLS
#endif
