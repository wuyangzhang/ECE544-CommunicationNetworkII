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

/***********************************************************************
* MF_VirtualGuidMap.cc
* Emulates GNRS functionality 
* Maintains two Hashtables : virtual guid -> true guid  AND true guid -> virtual guid   
 
*      Author: Aishwarya Babu
************************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include <pthread.h>

#include <vector>

#include "mffunctions.hh"

#include "mfvnguidmap.hh"
#define TMP_LINE_SIZE 1000


CLICK_DECLS

MF_VNGUIDmap::MF_VNGUIDmap(uint32_t myGuid, uint32_t myVirtualGuid, uint32_t virtualNetworkGuid) :
		logger(MF_Logger::init()), _virtualToTrueTable(), _trueToVirtualTable(){
    my_guid = myGuid;
    my_virtual_guid = myVirtualGuid;
    virtual_network_guid = virtualNetworkGuid;
}

MF_VNGUIDmap::~MF_VNGUIDmap() {
}

void MF_VNGUIDmap::initialize(String config_file) {
    parseFile(config_file);
    printVirtualToTrueMap();
    printTrueToVirtualMap();
}

/*!
  Parses file which has the virtual to true guid mappings and populates the two maps  
*/
void MF_VNGUIDmap::parseFile(String config_file){

    FILE * file = fopen(config_file.c_str(), "r");

    if(!file){
        logger.log(MF_CRITICAL,
                "virtual_guid_map: Could not open %s", config_file.c_str());
        //continue without topology enforcement
        return ;
    }
    logger.log(MF_INFO,
            "virtual_guid_map: Using topology file '%s'", config_file.c_str());

    int res;
    float ASRweight;
    uint32_t value;
    uint32_t virtualGUID, trueGUID;
    int neighbIt = 0;
    char metric[TMP_LINE_SIZE];
    char identifier[TMP_LINE_SIZE];

    //Assume contents of file have fixed ordering - Generalize and cater to other cases later
    while(1) {
        if((res = fscanf(file, "%s %u\n", identifier, &value)) == 2) {
            if(strcmp(identifier, "num_nodes")==0) {
                logger.log(MF_INFO, "virtual_guid_map: identifier %s is %u", identifier, value);
                num_nodes = value;
            }
        }
        for(int i = 0; i < num_nodes; i++)
        {
            if((res = fscanf(file, "%u %u\n", &virtualGUID, &trueGUID)) == 2) {
                logger.log(MF_INFO, "\t \t neighb %u %u", virtualGUID, trueGUID);
                insertVirtualToTrueMap(virtualGUID, trueGUID);
                insertTrueToVirtualMap(trueGUID, virtualGUID);
            }
        }
        if(feof(file))
            break;
    }//end while(1)
}

void MF_VNGUIDmap::printVirtualToTrueMap() {
  logger.log(MF_INFO, "virtual_guid_map:----------Virtual to True -----------------");
  for (virtualToTrue_map::const_iterator iter = _virtualToTrueTable.begin(); iter != _virtualToTrueTable.end(); ++iter){
    uint32_t guid = iter.key();
    virtualToTrue_t *node = iter.value();
    logger.log(MF_INFO, "\t \t  %u %d",guid, node->trueGUID);
  }
}

void MF_VNGUIDmap::printTrueToVirtualMap() {
  logger.log(MF_INFO, "virtual_guid_map:----------True to Virtual -----------------");
  for (trueToVirtual_map::const_iterator iter = _trueToVirtualTable.begin(); iter != _trueToVirtualTable.end(); ++iter){
    uint32_t guid = iter.key();
    trueToVirtual_t *node = iter.value();
    logger.log(MF_INFO, "\t \t  %u %d",guid, node->virtualGUID);
  }
}


//Setters
void MF_VNGUIDmap::setSelfVirtualGUID(uint32_t selfVirtualGUID) {
    my_virtual_guid = selfVirtualGUID;
}

void MF_VNGUIDmap::setVirtualNetworkGUID(uint32_t virtualNetworkGUID) {
    virtual_network_guid = virtualNetworkGUID;
}

//Getters
uint32_t MF_VNGUIDmap::getTrueGUID(uint32_t virtualGUID) {
    uint32_t trueGUID = -1;
    virtualToTrue_t temp;
    virtualToTrue_map::iterator it = _virtualToTrueTable.find(virtualGUID);
    if(it != _virtualToTrueTable.end()) {
        temp = *(it.value());
        trueGUID = temp.trueGUID;
    }
    //Iterator through entries of hash table .. need to access the value->trueGUID
    return trueGUID;
}

uint32_t MF_VNGUIDmap::getVirtualGUID(uint32_t trueGUID) {
    uint32_t virtualGUID = -1;
    trueToVirtual_t temp;
    trueToVirtual_map::iterator it = _trueToVirtualTable.find(trueGUID);
    if(it != _trueToVirtualTable.end()) {
        temp = *(it.value());
        virtualGUID = temp.virtualGUID;
    }
    //Iterator through entries of hash table .. need to access the value->trueGUID
    return virtualGUID;
}

virtualToTrue_map* MF_VNGUIDmap::getVirtualToTrueMap()
{
    return &_virtualToTrueTable;
}

trueToVirtual_map* MF_VNGUIDmap::getTrueToVirtualMap()
{
    return &_trueToVirtualTable;
}

//TODO: why here?
int MF_VNGUIDmap::getAlgorithmIndicator() {
   return algorithm_indicator;
}


//Inserts
void MF_VNGUIDmap::insertVirtualToTrueMap(uint32_t virtualGUID, uint32_t trueGUID) {
    virtualToTrue_t * temp = new virtualToTrue_t();
    temp->trueGUID = trueGUID;
    _virtualToTrueTable.set(virtualGUID, temp);

}

void MF_VNGUIDmap::insertTrueToVirtualMap(uint32_t trueGUID, uint32_t virtualGUID) {
    trueToVirtual_t * temp = new trueToVirtual_t();
    temp->virtualGUID = virtualGUID;
    _trueToVirtualTable.set(trueGUID, temp);
}


CLICK_ENDDECLS

ELEMENT_PROVIDES(MF_VNGUIDmap)
