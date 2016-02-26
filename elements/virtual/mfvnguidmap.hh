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

#ifndef MF_VN_GUID_MAP_HH
#define MF_VN_GUID_MAP_HH

#include <click/hashtable.hh>
#include "mflogger.hh"
#include "mfvirtualctrlmsg.hh"
//#include <queue>
//#include <vector>
/*
   Virtual->True Map:      VM GUID | TRUE GUID (Only the virtual neighbors of the VM node)
   If for a specific destination VM GUID the virtual -> true map doesn't have an entry the node queries the gnrs. 

 */

//TODO refactoring: now an object, not an element

CLICK_DECLS


//TODO why are these structures??
typedef struct virtualToTrue {
  uint32_t trueGUID;
}virtualToTrue_t;

typedef struct trueToVirtual {
  uint32_t virtualGUID;
}trueToVirtual_t;

//std::priority_queue<struct virtualRoutingTableEntry_t, std::vector<struct virtualRoutingTableEntry_t>, CompareVlinkValues > pq;

typedef HashTable<uint32_t, virtualToTrue_t*> virtualToTrue_map;
typedef HashTable<uint32_t, trueToVirtual_t*> trueToVirtual_map;

class MF_VNGUIDmap {

  public:
    MF_VNGUIDmap(uint32_t, uint32_t, uint32_t);
    ~MF_VNGUIDmap(); 

    const char* class_name() const        { return "MF_VirtualGUIDmap"; }
    //        void run_timer(Timer*);

    void initialize(String);

    void parseFile(String config_file);
    void printVirtualToTrueMap();
    void printTrueToVirtualMap();

    void insertVirtualToTrueMap(uint32_t virtualGUID, uint32_t trueGUID); //needs to query GNRS later 
    void insertTrueToVirtualMap(uint32_t trueGUID, uint32_t virtualGUID); //needs to query GNRS later 

    //Setters
    void setSelfVirtualGUID(uint32_t selfVirtualGUID);
    void setVirtualNetworkGUID(uint32_t virtualNetworkGUID);

    //Getters 
    uint32_t getTrueGUID(uint32_t);
    uint32_t getVirtualGUID(uint32_t);
    int getAlgorithmIndicator();

    virtualToTrue_map* getVirtualToTrueMap();
    trueToVirtual_map* getTrueToVirtualMap();

  private:
    virtualToTrue_map _virtualToTrueTable;
    trueToVirtual_map _trueToVirtualTable;

    uint32_t virtual_network_guid;
    uint32_t my_virtual_guid;
    uint32_t my_guid;
    uint32_t num_nodes;

    int algorithm_indicator;
    int file_size_asr;

    MF_Logger logger;
};

CLICK_ENDDECLS
#endif
