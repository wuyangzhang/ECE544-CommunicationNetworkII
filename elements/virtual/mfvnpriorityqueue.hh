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

#ifndef MF_VN_PRIORITY_QUEUE_HH
#define MF_VN_PRIORITY_QUEUE_HH

#define MAX_PATH_LENGTH 50

#include <click/hashtable.hh>
#include "mflogger.hh"
#include <queue>
#include <vector>

CLICK_DECLS

typedef struct virtualRoutingTableEntry {
	uint32_t vlinkCost;
	uint32_t nodeMetric;
//	uint32_t linkMetric;
	uint32_t nextHopGUID;
	uint32_t virtualPath[MAX_PATH_LENGTH];
	int pathLength;
	uint32_t epoch;

}virtualRoutingTableEntry_t;

class CompareVlinkValues {
	public:
		bool operator()(virtualRoutingTableEntry_t &v1, virtualRoutingTableEntry_t &v2) {
			return (v1.vlinkCost > v2.vlinkCost);
		}
};


class MF_VNPriorityQueue {

	public:
		MF_VNPriorityQueue();
		~MF_VNPriorityQueue();


		void insertIntoPriorityQueue(virtualRoutingTableEntry_t entry);
		//Setters
		void setNodeMetric(double nodeM){nodeMetric = nodeM;}

		//Getters
		double getNodeMetric(){return nodeMetric;}


	private:
		std::priority_queue<virtualRoutingTableEntry_t, std::vector<virtualRoutingTableEntry_t>, CompareVlinkValues > pq;
		uint32_t my_guid;
		double nodeMetric;
		MF_Logger logger;
};

CLICK_ENDDECLS
#endif
