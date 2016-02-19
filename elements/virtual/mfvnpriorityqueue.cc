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
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <pthread.h>

#include <vector>

#include "mffunctions.hh"

#include "mfvnpriorityqueue.hh"

CLICK_DECLS
MF_VNPriorityQueue::MF_VNPriorityQueue() : logger(MF_Logger::init()){
    nodeMetric = 0.3;
}

MF_VNPriorityQueue::~MF_VNPriorityQueue() {
}

/*
int MF_VirtualPriorityQueue::configure(Vector<String> &conf, ErrorHandler *errh) {

    logger.log(MF_INFO, "virtualRT: Inside configure of virtualTable \n");
    if (cp_va_kparse(conf, this, errh,
                "MY_GUID", cpkP+cpkM, cpInteger, &my_guid, 
                cpEnd) < 0) {
        return -1;
    }
    return 0; 
} 
*/
void MF_VNPriorityQueue::insertIntoPriorityQueue(virtualRoutingTableEntry_t entry){
    pq.push(entry);
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(MF_VNPriorityQueue)
