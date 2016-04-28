/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>

#include "dataModule.hh"
#include "packet.hh"

CLICK_DECLS 

DataModule::DataModule(){}

DataModule::~DataModule(){}

int 
DataModule::initialize(ErrorHandler* errh){
    return 0;
}


int
DataModule::configure(Vector<String>&conf, ErrorHandler* errh){
	if (cp_va_kparse(conf, this, errh,
           "ROUTING_TABLE", cpkP+cpkM, cpElement, &routingTable,
    		cpEnd) < 0) {
  	 	return -1;
  	}
  	return 0;
}


/* data module 
 * receiving a packet. 
 * check destination -> get forwarding port -> send out 
 *	uint8_t type;
 * 	uint16_t sourceAddr;
 * 	uint8_t k_value;
 * 	uint8_t sequenceNumber;
 * 	uint16_t destinationAddr1;
 * 	uint16_t destinationAddr2;
 * 	uint16_t destinationAddr3;
 * 	uint16_t length;
 *
 */


void 
DataModule::push(int port, Packet *packet) {

	struct DataPacket *dataPacket = (struct DataPacket *) packet->data();
  uint16_t DestAddr1 = dataPacket->destinationAddr1;
  uint16_t DestAddr2 = dataPacket->destinationAddr2;
  uint16_t DestAddr3 = dataPacket->destinationAddr3;

  if(dataPacket->k_value == 1){

      /* UNICAST: find the minmimum cost */
      Vector<uint16_t> Addr = sortCost(this->routingTable, DestAddr1, DestAddr2, DestAddr3);

      sendPacket1(Addr.at(0), packet);
  }

  if(dataPacket->k_value == 2){
      Vector<uint16_t> Addr = sortCost(this->routingTable, DestAddr1, DestAddr2, DestAddr3);
      if(Addr.size() == 2) {
          //check share path
          uint16_t addr1 = Addr.at(0);
          uint16_t addr2 = Addr.at(1);
          int sharedPort = checkSharedPort(addr1, addr2);
          if(sharedPort != -1) {
            sendPacket1(addr1, addr2, sharedPort, packet);
          }else {
            sendPacket1(addr1, packet);
            sendPacket1(addr2, packet);
          }
      }else if(Addr.size() == 3) {
          uint16_t addr1 = Addr.at(0);
          uint16_t addr2 = Addr.at(1);
          uint16_t addr3 = Addr.at(2);
          uint32_t cost1 = this->routingTable->routingTable.get(addr1)->cost;
          uint32_t cost2 = this->routingTable->routingTable.get(addr2)->cost;
          uint32_t cost3 = this->routingTable->routingTable.get(addr3)->cost;
          if(cost3 == cost2) {
            if(cost2 == cost1) {
              //check any 2 of 3
              int sharedPort12 = checkSharedPort(addr1, addr2);
              int sharedPort13 = checkSharedPort(addr1, addr3);
              int sharedPort23 = checkSharedPort(addr2, addr3);
              if(sharedPort12 != -1) {
                  sendPacket1(addr1, addr2, sharedPort12, packet);
              }else if(sharedPort13 != -1){
                  sendPacket1(addr1, addr3, sharedPort13, packet);
              }else if(sharedPort23 != -1){
                  sendPacket1(addr2, addr3, sharedPort23, packet);
              }else{
                sendPacket1(addr1, packet);
                sendPacket1(addr2, packet);
              }
            }else {
              //choose 1
              //check 1 and 3
              int sharedPort13 = checkSharedPort(addr1, addr3);
              if(sharedPort13 != -1) {
                sendPacket1(addr1, addr3, sharedPort13, packet);
              } else {
                //choose 1 and 2
                //check sharedPort
                int sharedPort12 = checkSharedPort(addr1, addr2);
                if(sharedPort12 != -1) {
                  sendPacket1(addr1, addr2, sharedPort12, packet);
                } else {
                  sendPacket1(addr1, packet);
                  sendPacket1(addr2, packet);
                }
              }
              
            }
          } else {
            //choose 1 and 2
            //check sharedPort between 1 and 2
            int sharedPort = checkSharedPort(addr1, addr2);
            if(sharedPort != -1) {
              sendPacket1(addr1, addr2, sharedPort, packet);
            } else {
              sendPacket1(addr1, packet);
              sendPacket1(addr2, packet);
            }
          }

      }else {
          click_chatter("[DataModule] Incompatibale packet, kill !");
          packet->kill();
      }
}

  if(dataPacket->k_value == 3) {
       int sharedPort = checkSharedPort(DestAddr1, DestAddr2, DestAddr3);
       if(sharedPort != -1) {
          //send packet to sharedPort
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());
          struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
          dataPacket1->k_value = 3;
          dataPacket1->destinationAddr1 = DestAddr1;
          dataPacket1->destinationAddr2 = DestAddr2;
          dataPacket1->destinationAddr3 = DestAddr3;
          output(sharedPort).push(p1);
          packet->kill();

       } else {
          int sharedPort12 = checkSharedPort(DestAddr1, DestAddr2);
          int sharedPort13 = checkSharedPort(DestAddr1, DestAddr3);
          int sharedPort23 = checkSharedPort(DestAddr2, DestAddr3);
          if(sharedPort12 != -1) {
              sendPacket2(DestAddr1, DestAddr2, DestAddr3, sharedPort12, packet);
          }else if(sharedPort13 != -1) {
              sendPacket2(DestAddr1, DestAddr3, DestAddr2, sharedPort13, packet);
          }else if(sharedPort23 != -1) {
              sendPacket2(DestAddr2, DestAddr3, DestAddr1, sharedPort23, packet);
          }else {
            //split separately
              sendPacket1(DestAddr1, packet);
              sendPacket1(DestAddr2, packet);
              sendPacket1(DestAddr3, packet);
          }
       }
  }



}


Vector<uint16_t>
DataModule::sortCost(RoutingTable* routingTable, uint16_t Addr1, uint16_t Addr2, uint16_t Addr3) {
  Vector<uint16_t> DestAddr;
  if(Addr3 == 0) {
    if(Addr2 == 0) {
      DestAddr.push_back(Addr1);
    }else {
      uint32_t cost1 = routingTable->routingTable.get(Addr1)->cost;
      uint32_t cost2 = routingTable->routingTable.get(Addr2)->cost;
      if(cost1 > cost2) {
        DestAddr.push_back(Addr2);
        DestAddr.push_back(Addr1);
      } else {
        DestAddr.push_back(Addr1);
        DestAddr.push_back(Addr2);
      }
    }
  } else {
    uint32_t cost1 = routingTable->routingTable.get(Addr1)->cost;
    uint32_t cost2 = routingTable->routingTable.get(Addr2)->cost;
    uint32_t cost3 = routingTable->routingTable.get(Addr3)->cost;
    if(cost1 > cost2) {
      if(cost2 > cost3) {
        //3 2 1
        DestAddr.push_back(Addr3);
        DestAddr.push_back(Addr2);
        DestAddr.push_back(Addr1);
      }else {
        if(cost1 > cost3) {
          //2 3 1
          DestAddr.push_back(Addr2);
          DestAddr.push_back(Addr3);
          DestAddr.push_back(Addr1);
        } else {
          //2 1 3
          DestAddr.push_back(Addr2);
          DestAddr.push_back(Addr1);
          DestAddr.push_back(Addr3);
        }
      }
    } else {
      if(cost2 < cost3) {
        //1 2 3
        DestAddr.push_back(Addr1);
        DestAddr.push_back(Addr2);
        DestAddr.push_back(Addr3);
      } else {
        if(cost1 > cost3) {
          //3 1 2
          DestAddr.push_back(Addr3);
          DestAddr.push_back(Addr1);
          DestAddr.push_back(Addr2);
        } else {
          //1 3 2
          DestAddr.push_back(Addr1);
          DestAddr.push_back(Addr3);
          DestAddr.push_back(Addr2);
        }
      }
    } 
  }
  return DestAddr;
}


int
DataModule::checkSharedPort(uint16_t addr1, uint16_t addr2) {
    Vector<uint8_t> forwardingPortSet1 = this->routingTable->lookUpForwardingTable(addr1);
    Vector<uint8_t> forwardingPortSet2 = this->routingTable->lookUpForwardingTable(addr2);
    uint8_t bitmap1 = 0;
    uint8_t bitmap2 = 0; 

    for(Vector<uint8_t>::iterator it = forwardingPortSet1.begin(); it != forwardingPortSet1.end(); ++it){
          bitmap1 = bitmap1 | (1<<*it);
    }
    for(Vector<uint8_t>::iterator it = forwardingPortSet2.begin(); it != forwardingPortSet2.end(); ++it){
          bitmap2 = bitmap2 | (1<<*it);
    }
    uint8_t bitmapS = bitmap1 & bitmap2;
    int sharedPort = -1;
    if(bitmapS != 0){
          while(bitmapS != 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
    }
    return sharedPort;
}


int
DataModule::checkSharedPort(uint16_t addr1, uint16_t addr2, uint16_t addr3) {
    Vector<uint8_t> forwardingPortSet1 = this->routingTable->lookUpForwardingTable(addr1);
    Vector<uint8_t> forwardingPortSet2 = this->routingTable->lookUpForwardingTable(addr2);
    Vector<uint8_t> forwardingPortSet3 = this->routingTable->lookUpForwardingTable(addr3);
    uint8_t bitmap1 = 0;
    uint8_t bitmap2 = 0; 
    uint8_t bitmap3 = 0; 

    for(Vector<uint8_t>::iterator it = forwardingPortSet1.begin(); it != forwardingPortSet1.end(); ++it){
          bitmap1 = bitmap1 | (1<<*it);
    }
    for(Vector<uint8_t>::iterator it = forwardingPortSet2.begin(); it != forwardingPortSet2.end(); ++it){
          bitmap2 = bitmap2 | (1<<*it);
    }
    uint8_t bitmapS = bitmap1 & bitmap2 & bitmap3;
    int sharedPort = -1;
    if(bitmapS != 0){
          while(bitmapS != 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
    }
    return sharedPort;
}


void
DataModule::sendPacket1(uint16_t addr1, Packet* packet) {
  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
    memcpy(p1->data(), packet->data(),packet->length());
    struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
    dataPacket1->k_value = 1;
    dataPacket1->destinationAddr1 = addr1;
    dataPacket1->destinationAddr2 = 0;
    dataPacket1->destinationAddr3 = 0;
    Vector<uint8_t> forwardingPortSet1 = this->routingTable->lookUpForwardingTable(addr1);
    if(!forwardingPortSet1.empty()){
          output(forwardingPortSet1.front()).push(p1);
      }else{
        output(0).push(p1);
      }
  packet->kill();
}


void
DataModule::sendPacket1(uint16_t shared_addr1, uint16_t shared_addr2, int sharedPort, Packet* packet) {
  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
    memcpy(p1->data(), packet->data(),packet->length());
    struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();    
    dataPacket1->k_value = 2;
    dataPacket1->destinationAddr1 = shared_addr1;
    dataPacket1->destinationAddr2 = shared_addr2;
    dataPacket1->destinationAddr3 = 0;
    output(sharedPort).push(p1);
  packet->kill();
}


void
DataModule::sendPacket2(uint16_t shared_addr1, uint16_t shared_addr2, uint16_t addr3, int sharedPort, Packet* packet) {
  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
    memcpy(p1->data(), packet->data(),packet->length());
    struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
    WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
    memcpy(p2->data(), packet->data(),packet->length());
    struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
    dataPacket1->k_value = 2;
    dataPacket1->destinationAddr1 = shared_addr1;
    dataPacket1->destinationAddr2 = shared_addr2;
    dataPacket1->destinationAddr3 = 0;
    output(sharedPort).push(p1);

    dataPacket2->k_value = 1;
    dataPacket1->destinationAddr1 = addr3;
    dataPacket2->destinationAddr2 = 0;
    dataPacket2->destinationAddr3 = 0;

    Vector<uint8_t> forwardingPortSet1 = this->routingTable->lookUpForwardingTable(addr3);
    if(!forwardingPortSet1.empty()){
          output(forwardingPortSet1.front()).push(p2);
      }else{
        output(0).push(p2);
      }

  packet->kill();
}




CLICK_ENDDECLS 
EXPORT_ELEMENT(DataModule)

