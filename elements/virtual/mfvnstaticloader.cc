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
#include <click/hashtable.hh>
#include <click/element.hh>

#include "mfvnmanager.hh"
#include "mfvnstaticloader.hh"

CLICK_DECLS

MF_VNStaticLoader::MF_VNStaticLoader() : timer(this){

}

MF_VNStaticLoader::~MF_VNStaticLoader(){

}

int MF_VNStaticLoader::configure(Vector<String> &conf, ErrorHandler *errh){
    if (cp_va_kparse(conf, this, errh,
			"MY_VIRTUAL_GUID", cpkP+cpkM, cpInteger, &myVirtualGuid,
			"MY_VIRTUAL_NETWORK_GUID", cpkP+cpkM, cpInteger, &virtualNetworkGuid,
			"VIRTUAL_TOPO_FILE", cpkP+cpkM, cpString, &vtopoFile,
			"VIRTUAL_CONFIG_FILE", cpkP+cpkM, cpString, &configFile,
			"VIRTUAL_SERVICE_FILE", cpkP+cpkM, cpString, &serviceFile,
			"VN_MANAGER", cpkP+cpkM, cpElement, &_vnManager,
			"VN_LSA_HANDLER", cpkP+cpkM, cpElement, &_vnLSAHandler,
			"VN_NEIGHBOR_TABLE_COMPUTE", cpkP+cpkM, cpElement, &_vnNeighborTableCompute,
			"VN_ROUTING_TABLE_COMPUTE", cpkP+cpkM, cpElement, &_vnRoutingTableCompute,
        cpEnd) < 0) {
        return -1;
    }
    return 0;
}

int MF_VNStaticLoader::initialize(ErrorHandler *){
	timer.initialize(this);
	timer.schedule_after_sec(1);
	return 0;
}

void MF_VNStaticLoader::run_timer(Timer*){
	_vnManager->configureVirtualNetwork(myVirtualGuid, virtualNetworkGuid, configFile, vtopoFile, serviceFile);
	_vnLSAHandler->addVNTimer(virtualNetworkGuid);
	_vnNeighborTableCompute->addVNTimer(virtualNetworkGuid);
	_vnRoutingTableCompute->addVNTimer(virtualNetworkGuid);
}


CLICK_ENDDECLS

ELEMENT_REQUIRES(userlevel)
EXPORT_ELEMENT(MF_VNStaticLoader)
