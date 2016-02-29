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

#ifndef MF_VN_INFO_CONTAINER_HH
#define MF_VN_INFO_CONTAINER_HH

#include <pthread.h>

#include "mfvnroutingtable.hh"
#include "mfvnneighbortable.hh"
#include "mfvnguidmap.hh"
#include "mflogger.hh"

/*
 * Class comprising all components needed to define a virtual network
 * Virtual Topology manager has a hash table with key : VN guid and value : object of this class 
*/

CLICK_DECLS
class MF_VNInfoContainer {

    public:

	//Create new tables associated with the virtual network by passing the vconfig, service and virtual topology file
	MF_VNInfoContainer(uint32_t myGuid, uint32_t myVirtualGuid,
		    uint32_t virtualNetworkGuid, String configFile, String vtopoFile, String serviceFile);
    ~MF_VNInfoContainer();

    const char* class_name() const { return "MF_VNInfoContainer"; }
		
    MF_VNNeighborTable* getVNNeighborTable(){
    	return &_vneighbortable;
    }

    MF_VNRoutingTable* getVNRoutingTable(){
    	return &_vroutingtable;
    }

    MF_VNGUIDmap* getVNGUIDMap(){
    	return &_vguidmap;
    }

    MF_VNNeighborTable* getVNNeighborTableConcurrent(){
    	pthread_mutex_lock(&_vneighbortable_lock);
    	pthread_mutex_lock(&_access_lock);
    	_vneighbortable_taken = true;
    	pthread_mutex_unlock(&_access_lock);
		return &_vneighbortable;
	}

    MF_VNRoutingTable* getVNRoutingTableConcurrent(){
		pthread_mutex_lock(&_vroutingtable_lock);
		pthread_mutex_lock(&_access_lock);
		_vroutingtable_taken = true;
		pthread_mutex_unlock(&_access_lock);
		return &_vroutingtable;
	}

	MF_VNGUIDmap* getVNGUIDMapConcurrent(){
		pthread_mutex_lock(&_vguidmap_lock);
		pthread_mutex_lock(&_access_lock);
		_vguidmap_taken = true;
		pthread_mutex_unlock(&_access_lock);
		return &_vguidmap;
	}

	MF_VNNeighborTable* getVNNeighborTableConcurrentNB(){
		MF_VNNeighborTable *ret;
    	if( pthread_mutex_trylock(&_vneighbortable_lock)){
    		ret = NULL;
    	} else {
			pthread_mutex_lock(&_access_lock);
			_vneighbortable_taken = true;
			pthread_mutex_unlock(&_access_lock);
			ret = &_vneighbortable;
    	}
    	return ret;
	}

	MF_VNRoutingTable* getVNRoutingTableConcurrentNB(){
		MF_VNRoutingTable *ret;
		if( pthread_mutex_trylock(&_vroutingtable_lock)){
			ret = NULL;
		} else {
			pthread_mutex_lock(&_access_lock);
			_vroutingtable_taken = true;
			pthread_mutex_unlock(&_access_lock);
			ret = &_vroutingtable;
		}
		return ret;
	}

	MF_VNGUIDmap* getVNGUIDMapConcurrentNB(){
		MF_VNGUIDmap *ret;
		if( pthread_mutex_trylock(&_vguidmap_lock)){
			ret = NULL;
		} else {
			pthread_mutex_lock(&_access_lock);
			_vguidmap_taken = true;
			pthread_mutex_unlock(&_access_lock);
			ret = &_vguidmap;
		}
		return ret;
	}

	void returnVNNeighborTable() {
    	pthread_mutex_lock(&_access_lock);
    	if(_vneighbortable_taken == true){
        	pthread_mutex_unlock(&_vneighbortable_lock);
    	}
    	pthread_mutex_unlock(&_access_lock);
	}

	void returnVNRoutingTable() {
		pthread_mutex_lock(&_access_lock);
		if(_vroutingtable_taken == true){
			pthread_mutex_unlock(&_vroutingtable_lock);
		}
		pthread_mutex_unlock(&_access_lock);
	}

	void returnVNGUIDMap() {
		pthread_mutex_lock(&_access_lock);
		if(_vguidmap_taken == true){
			pthread_mutex_unlock(&_vguidmap_lock);
		}
		pthread_mutex_unlock(&_access_lock);
	}

	bool isInitialized() const {
		return initialized;
	}

	uint32_t getMyVirtualGuid() const {
		return myVirtualGuid;
	}

	uint32_t getVirtualNetworkGuid() const {
		return virtualNetworkGuid;
	}

	uint32_t getLspSeqNo() const {
		return _lsp_seq_no;
	}

	void setLspSeqNo(uint32_t lspSeqNo) {
		_lsp_seq_no = lspSeqNo;
	}

    private:

    MF_Logger logger;
    MF_VNNeighborTable _vneighbortable; //new
    MF_VNRoutingTable _vroutingtable; //new
    MF_VNGUIDmap _vguidmap; //new
    uint32_t myVirtualGuid;
    uint32_t virtualNetworkGuid;
    uint32_t _lsp_seq_no;

    pthread_mutex_t _vroutingtable_lock;
    pthread_mutex_t _vneighbortable_lock;
    pthread_mutex_t _vguidmap_lock;
    pthread_mutex_t _access_lock;
    bool _vroutingtable_taken;
	bool _vneighbortable_taken;
	bool _vguidmap_taken;
   
    bool initialized;
};

CLICK_ENDDECLS
#endif
