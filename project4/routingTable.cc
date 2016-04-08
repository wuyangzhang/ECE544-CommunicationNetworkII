/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>

#include "routingTable.hh" 
#include "packet.hh"

CLICK_DECLS 
RoutingTable::RoutingTable(){}

RoutingTable::~RoutingTable(){}

int 
RoutingTable::initialize(){
    return 0;
}

void 
RoutingTable::push(int port, Packet *packet) {
	
}

void 
RoutingTable::updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop){
  
}


void 
RoutingTable::updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port){

}

void
RoutingTable::updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop, const uint8_t sharedPath){
	
}


void
RoutingTable::updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port, const uint8_t sharedPath){

}


/* computeRoutingTable()
 * when a new path to destination presents,
 * if destionation is not in the list, add it
 * new cost < current cost, update value
 * new cost == current cost, record new path
 */
void 
RoutingTable::computeRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop){

  struct routingTableParam* rtp = new struct routingTableParam(); 
    rtp->nextHop = Vector<uint16_t>();
    rtp->cost = cost;

    if(!this->routingTable[sourceAddr]){
    	/* add new destination address */
    	rtp->nextHop.push_back(nextHop);
        this->forwardingTable.set(sourceAddr, *rtp);
    }else if(cost < this->routingTable.get(sourceAddr).cost){
    	/* update the cost of destination */
    	rtp->nextHop.push_back(nextHop);
        this->routingTable.set(sourceAddr, *rtp);
    }else if(cost == this->routingTable.get(sourceAddr).cost){
    	/* fetch all current next hop followed by a new next hop, build a new next hop structure */
    	for(Vector<uint16_t>::iterator it = this->routingTable.get(sourceAddr).nextHop.begin(); it != this->routingTable.get(sourceAddr).nextHop.end(); ++it){
    		rtp->nextHop.push_back(it);
    	}

    	rtp->nextHop.push_back(nextHop);
    }
    /* set hop count of this source address */
    this->routingTable.get(sourceAddr).hopCount = this->routingTable.get(sourceAddr).nextHop.size();
    delete rtp;
}


void
RoutingTable::computeForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port){
  struct forwardingTableParam* ftp = new struct forwardingTableParam(); 
    ftp->port = Vector<uint8_t>();
    ftp->cost = cost;

    if(!this->forwardingTable[destAddr]){
    	/* add new destination address */
    	ftp->port.push_back(port);
        this->forwardingTable.set(sourceAddr, *ftp);
    }else if(cost < this->forwardingTable.get(sourceAddr).cost){
    	/* update the cost of destination */
    	ftp->port.push_back(port);
        this->forwardingTable.set(sourceAddr, *ftp);
    }else if(cost == this->forwardingTable.get(sourceAddr).cost){
    	/* fetch all current next hop followed by a new next hop, build a new next hop structure */
    	for(Vecotr<uint8_t>::iterator it = this->forwardingTable.get(sourceAddr).port.begin(); it != this->forwardingTable.get(sourceAddr).port.end(); ++it){
    		ftp->nextHop.push_back(it);
    	}

    	ftp->nextHop.push_back(port);
    }

     /* set hop count of this source address */
    this->forwardingTable.get(sourceAddr).portCount = this->forwardingTable.get(sourceAddr).port.size();
    delete ftp;
}


int 
RoutingTable::lookUpForwardingTable(uint32_t destAddr){

    int matchPort = -1;
    
    return matchPort;
}


void 
RoutingTable::forwardingPacket(Packet *p, int port){
    output(port).push(p);
    click_chatter("[Router] forwarding packet to port %d", port);
}

void 
RoutingTable::printRoutingTable(){

}


void
RoutingTable::printForwardingTable(){

}

CLICK_ENDDECLS 
EXPORT_ELEMENT(RoutingTable)

