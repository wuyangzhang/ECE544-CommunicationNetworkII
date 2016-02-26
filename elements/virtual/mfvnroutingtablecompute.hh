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

#ifndef MF_VN_ROUTING_TABLE_COMPUTE_HH
#define MF_VN_ROUTING_TABLE_COMPUTE_HH

/* virtual forwarding table recompute interval */
#define VRT_UPDATE_PERIOD_MSECS 1000

/* force recompute of FT after max skips */
#define MAX_VRT_SKIPPED_UPDATES 2

#include <click/hashtable.hh>
#include "mflogger.hh"
//#include <queue>
//#include <vector>
#include <pthread.h>

#include "mfvnpriorityqueue.hh"
#include "mfroutingtable.hh"
#include "mfvnneighbortable.hh"
#include "mfvnguidmap.hh"
#include "mfvnmanager.hh"
#include "mfvninfocontainer.hh"

/*
   Virtual->True Map:      VM GUID | TRUE GUID (Only the virtual neighbors of the VM node)
   If for a specific destination VM GUID the virtual -> true map doesn't have an entry the node queries the gnrs. 

   Virtual Routing table:  VM GUID | ASR Metric | VM GUID Next hop
 */

CLICK_DECLS

class MF_VNRoutingTableCompute : public Element {

	public:
		MF_VNRoutingTableCompute();
		~MF_VNRoutingTableCompute();

		const char* class_name() const        { return "MF_VNRoutingTableCompute"; }
		//        const char* port_count() const        { return PORTS_0_0; }
		//        const char* processing() const        { return AGNOSTIC; }
		MF_VNRoutingTableCompute* clone() const        { return new MF_VNRoutingTableCompute; }

		int initialize(ErrorHandler *);
		int configure(Vector<String>&, ErrorHandler *);
		static void callback(Timer *t, void *user_data);
		void run_timer(Timer *t, void *user_data);

		//Update flag to recompute virtual routing table using djikstra at the advent of a new virtual network state packet
		void req_vrt_update(); 

        void computeForwardtable(uint32_t vnGUID);

        void addVNTimer(uint32_t vnGUID);
        void removeVNTimer(uint32_t vnGUID);

	private:
		MF_VNManager * _vnManager;

		typedef struct timerInfo {
			MF_VNRoutingTableCompute *handler;
			uint32_t guid;
			Timer *timer;
		}timerInfo_t;
		typedef HashTable<uint32_t, timerInfo_t*> timerMap_t;

		timerMap_t timerMap;

		MF_Logger logger;
};

CLICK_ENDDECLS
#endif
