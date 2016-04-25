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
  Vector<uint8_t> forwardingPortSet1;
  Vector<uint8_t> forwardingPortSet2;
  Vector<uint8_t> forwardingPortSet3;

  uint8_t destinationAddr1 = dataPacket->destinationAddr1;
  uint8_t destinationAddr2 = dataPacket->destinationAddr2;
  uint8_t destinationAddr3 = dataPacket->destinationAddr3;

  if(dataPacket->k_value == 1){

      /* UNICAST: find the minmimum cost */
      //check and set valid address
      if(dataPacket->destinationAddr2 == 0 && dataPacket->destinationAddr3 == 0) {
        
        /*only destinationAddr1 is valid, do nothing*/

      } else if(dataPacket->destinationAddr2 == 0) {

        //compare cost between destinationAddr1 and destinationAddr3 
        uint32_t cost1 = this->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost3 = this->routingTable.get(dataPacket->destinationAddr3)->cost;
        if(cost1 > cost3) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr3;
          dataPacket->destinationAddr3 = 0;
        } else {
          dataPacket->destinationAddr3 = 0;
        }

      } else if (dataPacket->destinationAddr3 == 0) {

        //compare cost between destinationAddr1 and destinationAddr2
        uint32_t cost1 = this->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost2 = this->routingTable.get(dataPacket->destinationAddr2)->cost;
        if(cost1 > cost2) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket->destinationAddr2 = 0;
        } else {
          dataPacket->destinationAddr2 = 0;
        }

      } else {

        //compare cost between destinationAddr1, destinationAddr2 and destinationAddr3 
        uint32_t cost1 = this->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost2 = this->routingTable.get(dataPacket->destinationAddr2)->cost;
        if(cost1 > cost2) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket->destinationAddr2 = 0;
        } else {
          dataPacket->destinationAddr2 = 0;
        }

        cost1 = this->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost3 = this->routingTable.get(dataPacket->destinationAddr3)->cost;
        if(cost1 > cost3) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr3;
          dataPacket->destinationAddr3 = 0;
        } else {
          dataPacket->destinationAddr3 = 0;
        }

      }

      //now only destinationAddr1 is valid
      forwardingPortSet1 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr1);
      if(!forwardingPortSet1.empty()){
          output(forwardingPortSet1.front()).push(packet);
      }else{
        output(0).push(packet);
      }
  }

  uint8_t bitmap1 = 0;
  uint8_t bitmap2 = 0;
  uint8_t bitmap3 = 0;

  if(dataPacket->k_value == 2){
      forwardingPortSet1 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr1);
      forwardingPortSet2 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr2);

      for(Vector<uint8_t>::iterator it = forwardingPortSet1.begin(); it != forwardingPortSet1.end(); ++it){
         bitmap1 = bitmap1 | (1<<*it);
      }
      for(Vector<uint8_t>::iterator it = forwardingPortSet2.begin(); it != forwardingPortSet2.end(); ++it){
         bitmap2 = bitmap2 | (1<<*it);
      }
      uint8_t bitmapS = bitmap1 & bitmap2;
      /* not share path */

      if(bitmapS == 0){
         click_chatter("[DataModule] split packet from %d to destinationAddr1 %d, destinationAddr2 %d",dataPacket->sourceAddr, destinationAddr1, destinationAddr2);
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

         struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
         struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
         dataPacket1->k_value = 1;
         dataPacket2->k_value = 1;
         //dataPacket1->destinationAddr1 = destinationAddr1;
         dataPacket2->destinationAddr1 = destinationAddr2;
         output(forwardingPortSet1.front()).push(p1);
         output(forwardingPortSet2.front()).push(p2);
         packet->kill();
      }else{
         /* two destinations share paths */

          int sharedPort = 0;
          while( (bitmapS & 1) == 0){
             bitmapS = bitmapS >> 1;
             sharedPort++;
          }
          click_chatter("[DataModule] forwarding packet to shard port %d", sharedPort);
          output(sharedPort).push(packet);
      }
  }

  if(dataPacket->k_value == 3){
      forwardingPortSet1 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr1);
      forwardingPortSet2 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr2);
      forwardingPortSet3 = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr3);

      for(Vector<uint8_t>::iterator it = forwardingPortSet1.begin(); it != forwardingPortSet1.end(); ++it){
         bitmap1 = bitmap1 | (1<<*it);
      }
      for(Vector<uint8_t>::iterator it = forwardingPortSet2.begin(); it != forwardingPortSet2.end(); ++it){
         bitmap2 = bitmap2 | (1<<*it);
      }
      for(Vector<uint8_t>::iterator it = forwardingPortSet3.begin(); it != forwardingPortSet3.end(); ++it){
         bitmap3 = bitmap3 | (1<<*it);
      }
      uint8_t bitmapS = bitmap1 & bitmap2 & bitmap3;

      
      /* no shared path */
      if(bitmapS == 0){
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          WritablePacket *p3 = Packet::make(0,0,packet->length(),0);
          memcpy(p3->data(), packet->data(),packet->length());

         struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
         struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
         struct DataPacket *dataPacket3 = (struct DataPacket*) p3->data();
         dataPacket1->k_value = 1;
         dataPacket2->k_value = 1;
         dataPacket3->k_value = 1;
         dataPacket1->destinationAddr1 = destinationAddr1;
         dataPacket2->destinationAddr1 = destinationAddr2;
         dataPacket3->destinationAddr1 = destinationAddr3;

         output(forwardingPortSet1.front()).push(p1);
         output(forwardingPortSet2.front()).push(p2);
         output(forwardingPortSet3.front()).push(p3);

      }

      /* three destinations share path */
      bitmapS = 0;
      bitmapS = bitmap1 | bitmap2 | bitmap3 ;
      if(bitmapS == 1){
          int sharedPort = 0;
          while( (bitmapS & 1) == 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
           output(sharedPort).push(packet);
          
      }

      /* two destinations share path */
      bitmapS = 0;
      bitmapS = bitmap1 & bitmap2;
      if(bitmapS == 1){
          int sharedPort = 0;
          while( (bitmapS & 1) == 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          WritablePacket *p3 = Packet::make(0,0,packet->length(),0);
          memcpy(p3->data(), packet->data(),packet->length());

           struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
           struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
           struct DataPacket *dataPacket3 = (struct DataPacket*) p3->data();
           dataPacket1->k_value = 2;
           dataPacket2->k_value = 2;
           dataPacket3->k_value = 1;
  
           dataPacket3->destinationAddr1 = dataPacket->destinationAddr3;
           output(sharedPort).push(p1);
           output(sharedPort).push(p2);
           output(forwardingPortSet3.front()).push(p3);
      }

      bitmapS = 0;
      bitmapS = bitmap1 & bitmap3;
       if(bitmapS == 1){
          int sharedPort = 0;
          while( (bitmapS & 1) == 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          WritablePacket *p3 = Packet::make(0,0,packet->length(),0);
          memcpy(p3->data(), packet->data(),packet->length());

           struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
           struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
           struct DataPacket *dataPacket3 = (struct DataPacket*) p3->data();
           dataPacket1->k_value = 2;
           dataPacket2->k_value = 1;
           dataPacket3->k_value = 2;
           dataPacket1->destinationAddr2 = dataPacket->destinationAddr3;
           dataPacket2->destinationAddr1 = dataPacket->destinationAddr3;
           dataPacket3->destinationAddr2 = dataPacket->destinationAddr3;
           output(sharedPort).push(p1);
           output(forwardingPortSet2.front()).push(p2);
           output(sharedPort).push(p3);
        }

      bitmapS = 0;
      bitmap3 = bitmap2 & bitmap3;
        if(bitmapS == 1){
          int sharedPort = 0;
          while( (bitmapS & 1) == 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          WritablePacket *p3 = Packet::make(0,0,packet->length(),0);
          memcpy(p3->data(), packet->data(),packet->length());
           struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
           struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
           struct DataPacket *dataPacket3 = (struct DataPacket*) p3->data();
           dataPacket1->k_value = 1;
           dataPacket2->k_value = 2;
           dataPacket3->k_value = 2;
           dataPacket2->destinationAddr1 = dataPacket->destinationAddr3;
           dataPacket3->destinationAddr1 = dataPacket->destinationAddr3;
           output(forwardingPortSet3.front()).push(p1);
           output(sharedPort).push(p2);
           output(sharedPort).push(p3);
        }

      packet->kill();
  }



}


CLICK_ENDDECLS 
EXPORT_ELEMENT(DataModule)

