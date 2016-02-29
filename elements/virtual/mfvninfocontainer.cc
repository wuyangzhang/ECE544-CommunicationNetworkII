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
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "mfvninfocontainer.hh"


CLICK_DECLS
MF_VNInfoContainer::MF_VNInfoContainer(uint32_t myGuid, uint32_t myVirtualGuid,
		uint32_t virtualNetworkGuid, String configFile, String vtopoFile, String serviceFile) :
		logger(MF_Logger::init()),  _vguidmap(myGuid, myVirtualGuid, virtualNetworkGuid),
		_vneighbortable(myGuid, virtualNetworkGuid, myVirtualGuid), _vroutingtable(){
	initialized = false;
	pthread_mutex_init(&_vroutingtable_lock, NULL);
	pthread_mutex_init(&_vneighbortable_lock, NULL);
	pthread_mutex_init(&_vguidmap_lock, NULL);
	pthread_mutex_init(&_access_lock, NULL);
	_vroutingtable_taken = false;
	_vneighbortable_taken = false;
	_vguidmap_taken = false;
	_lsp_seq_no = 0;
	logger.log(MF_DEBUG, "virtual_network_information: request to create new virtual network");
	this->myVirtualGuid = myVirtualGuid;
	this->virtualNetworkGuid = virtualNetworkGuid;

	_vguidmap.initialize(configFile);
	_vneighbortable.initialize(vtopoFile, &_vguidmap);
	_vroutingtable.initialize(serviceFile);

	//Initialize
	initialized = true;
}

MF_VNInfoContainer::~MF_VNInfoContainer(){

}

CLICK_ENDDECLS

ELEMENT_PROVIDES(MF_VNInfoContainer)
