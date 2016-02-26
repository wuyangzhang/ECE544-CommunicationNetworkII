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
#include "mfvninfocontainer.hh"

CLICK_DECLS

MF_VNManager::MF_VNManager():logger(MF_Logger::init()){

}

MF_VNManager::~MF_VNManager(){

}

int MF_VNManager::configure(Vector<String> &conf, ErrorHandler *errh){
    if (cp_va_kparse(conf, this, errh,
    		"MY_GUID", cpkP+cpkM, cpInteger, &myGuid,
        cpEnd) < 0) {
        return -1;
    }
    return 0;
}

int MF_VNManager::configureVirtualNetwork( uint32_t myVirtualGuid,
    uint32_t virtualNetworkGuid, String configFile, String vtopoFile, String serviceFile) {
	logger.log(MF_DEBUG, "MF_VNManager: request to create new virtual network");
	if(_hashTable.get(virtualNetworkGuid) == _hashTable.default_value()) {
		MF_VNInfoContainer *newContainer = new MF_VNInfoContainer(myGuid, myVirtualGuid, virtualNetworkGuid, configFile, vtopoFile, serviceFile);
		_hashTable.set(virtualNetworkGuid, newContainer);
		return 0;
	} else {
		return -1;
	}
}

int MF_VNManager::addVirtualNetwork(MF_VNInfoContainer *newContainer){
	if(newContainer->isInitialized() && _hashTable.get(newContainer->getVirtualNetworkGuid()) == _hashTable.default_value()) {
		_hashTable.set(newContainer->getVirtualNetworkGuid(), newContainer);
		return 0;
	} else {
		return -1;
	}
}

MF_VNInfoContainer *MF_VNManager::getVNContainer(uint32_t guid){
	MF_VNInfoContainer *ret = _hashTable.get(guid);
	if(ret == _hashTable.default_value()) {
		logger.log(MF_WARN, "MF_VNManager: no VN with GUID %u", guid);
		return NULL;
	}
	return ret;
}

CLICK_ENDDECLS

ELEMENT_REQUIRES(userlevel)
EXPORT_ELEMENT(MF_VNManager)
