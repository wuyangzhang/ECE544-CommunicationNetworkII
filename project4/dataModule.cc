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
        uint32_t cost1 = this->routingTable->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost3 = this->routingTable->routingTable.get(dataPacket->destinationAddr3)->cost;
        if(cost1 > cost3) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr3;
          dataPacket->destinationAddr3 = 0;
        } else {
          dataPacket->destinationAddr3 = 0;
        }

      } else if (dataPacket->destinationAddr3 == 0) {

        //compare cost between destinationAddr1 and destinationAddr2
        uint32_t cost1 = this->routingTable->routingTable.get(dataPacket->destinationAddr1)->cost;
        uint32_t cost2 = this->routingTable->routingTable.get(dataPacket->destinationAddr2)->cost;
        if(cost1 > cost2) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket->destinationAddr2 = 0;
        } else {
          dataPacket->destinationAddr2 = 0;
        }

      } else {

        //compare cost between destinationAddr1, destinationAddr2 and destinationAddr3 
        uint32_t cost1 = this->routingTable->routingTable.get(dataPacket->destinationAddr1)->cost;
        click_chatter("[DataModule]cost to destinationAddr1 is %d", cost1);
        uint32_t cost2 = this->routingTable->routingTable.get(dataPacket->destinationAddr2)->cost;
        click_chatter("[DataModule]cost to destinationAddr2 is %d", cost2);
        if(cost1 > cost2) {
          dataPacket->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket->destinationAddr2 = 0;
        } else {
          dataPacket->destinationAddr2 = 0;
        }

        cost1 = this->routingTable->routingTable.get(dataPacket->destinationAddr1)->cost;
        click_chatter("[DataModule]cost to destinationAddr1 is %d", cost1);
        uint32_t cost3 = this->routingTable->routingTable.get(dataPacket->destinationAddr3)->cost;
        click_chatter("[DataModule]cost to destinationAddr3 is %d", cost3);
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

      packet->kill();
  }

  uint8_t bitmap1 = 0;
  uint8_t bitmap2 = 0;
  uint8_t bitmap3 = 0;

  if(dataPacket->k_value == 2){
    /*
      check whether destination3 is 0 , if it is 0, just unicast to destination1 and destination 2
      if not , find shared path-> three share
                               -> two share
                               -> no share  
      between D1 and D2, D1 and D3 , D2 and D3
    */
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
      

      if(dataPacket->destinationAddr3 == 0) {
          uint8_t bitmapS = bitmap1 & bitmap2;
          if(bitmapS != 0) {
            int sharedPort = 0;
            while(bitmapS != 0){
              bitmapS = bitmapS >> 1;
              sharedPort++;
            }
            output(sharedPort).push(packet);
          } else {
            //no share path, split
            WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
            memcpy(p1->data(), packet->data(),packet->length());

            WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
            memcpy(p2->data(), packet->data(),packet->length());

           struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
           struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
           dataPacket1->k_value = 1;
           dataPacket2->k_value = 1;
           /*set destinationAddr2 and destinationAddr3 to be 0*/
           dataPacket1->destinationAddr1 = destinationAddr1;
           dataPacket1->destinationAddr2 = 0;
           dataPacket1->destinationAddr3 = 0;
           dataPacket2->destinationAddr1 = destinationAddr2;
           dataPacket2->destinationAddr2 = 0;
           dataPacket2->destinationAddr3 = 0;
           output(forwardingPortSet1.front()).push(p1);
           output(forwardingPortSet2.front()).push(p2);
           
          }
      } else {
          //choose 2 out of 3 destination
          //uint8_t bitmapS = bitmap1 & bitmap2 & bitmap3;
          uint32_t cost1 = this->routingTable->routingTable.get(dataPacket->destinationAddr1)->cost;
          uint32_t cost2 = this->routingTable->routingTable.get(dataPacket->destinationAddr2)->cost;
          uint32_t cost3 = this->routingTable->routingTable.get(dataPacket->destinationAddr3)->cost;
          if(cost1 < cost2) {
            if(cost2 < cost3) {
                //choose 1 and 2

                //check whether share same port
                uint8_t bitmapS = bitmap1 & bitmap2;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr2;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);

                } else {
                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());
                  WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                  memcpy(p2->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                  dataPacket1->k_value = 1;
                  dataPacket2->k_value = 1;

                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = 0;
                  dataPacket1->destinationAddr3 = 0;
                  dataPacket2->destinationAddr1 = destinationAddr2;
                  dataPacket2->destinationAddr2 = 0;
                  dataPacket2->destinationAddr3 = 0;

                  output(forwardingPortSet1.front()).push(p1);
                  output(forwardingPortSet2.front()).push(p2);

                }

            } else if(cost2 > cost3) {
              //choose 1 and 3
              //check whether share same port
                uint8_t bitmapS = bitmap1 & bitmap3;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr3;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);
                } else {
                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());
                  WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                  memcpy(p2->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                  dataPacket1->k_value = 1;
                  dataPacket2->k_value = 1;

                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = 0;
                  dataPacket1->destinationAddr3 = 0;
                  dataPacket2->destinationAddr1 = destinationAddr3;
                  dataPacket2->destinationAddr2 = 0;
                  dataPacket2->destinationAddr3 = 0;

                  output(forwardingPortSet1.front()).push(p1);
                  output(forwardingPortSet3.front()).push(p2);

                }
            } else {
              //cost1 < cost2 == cost3 
              //check whether D3 share nextHop with D1
              //if yes, choose 1 and 3
              uint8_t bitmapS = bitmap1 & bitmap3;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr3;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);
                } else { //if not, check whether 1 and 2 share path
                    uint8_t bitmapS = bitmap1 & bitmap2;
                    if(bitmapS != 0) {
                      int sharedPort = 0;
                      while(bitmapS != 0){
                        bitmapS = bitmapS >> 1;
                        sharedPort++;
                      }

                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      dataPacket1->k_value = 2;
                      dataPacket1->destinationAddr1 = destinationAddr1;
                      dataPacket1->destinationAddr2 = destinationAddr2;
                      dataPacket1->destinationAddr3 = 0;

                      output(sharedPort).push(p1);

                    } else {
                      //no share path, split
                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());
                      WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                      memcpy(p2->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                      dataPacket1->k_value = 1;
                      dataPacket2->k_value = 1;

                      dataPacket1->destinationAddr1 = destinationAddr1;
                      dataPacket1->destinationAddr2 = 0;
                      dataPacket1->destinationAddr3 = 0;
                      dataPacket2->destinationAddr1 = destinationAddr2;
                      dataPacket2->destinationAddr2 = 0;
                      dataPacket2->destinationAddr3 = 0;

                      output(forwardingPortSet1.front()).push(p1);
                      output(forwardingPortSet2.front()).push(p2);

                    }

                  }
              }
          } else if(cost1 > cost2){
            if(cost1 > cost3) {
              //choose 2 and 3
              //check whether share same port
                uint8_t bitmapS = bitmap2 & bitmap3;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr2;
                  dataPacket1->destinationAddr2 = destinationAddr3;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);

                } else {
                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());
                  WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                  memcpy(p2->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                  dataPacket1->k_value = 1;
                  dataPacket2->k_value = 1;

                  dataPacket1->destinationAddr1 = destinationAddr2;
                  dataPacket1->destinationAddr2 = 0;
                  dataPacket1->destinationAddr3 = 0;
                  dataPacket2->destinationAddr1 = destinationAddr3;
                  dataPacket2->destinationAddr2 = 0;
                  dataPacket2->destinationAddr3 = 0;

                  output(forwardingPortSet2.front()).push(p1);
                  output(forwardingPortSet3.front()).push(p2);

                }
            } else if (cost1 < cost3) {
              //choose 1 and 2
              //check whether share same port
                uint8_t bitmapS = bitmap1 & bitmap2;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr2;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);

                } else {
                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());
                  WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                  memcpy(p2->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                  dataPacket1->k_value = 1;
                  dataPacket2->k_value = 1;

                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = 0;
                  dataPacket1->destinationAddr3 = 0;
                  dataPacket2->destinationAddr1 = destinationAddr2;
                  dataPacket2->destinationAddr2 = 0;
                  dataPacket2->destinationAddr3 = 0;

                  output(forwardingPortSet1.front()).push(p1);
                  output(forwardingPortSet2.front()).push(p2);

                }
            } else {
              //check whether D1 share nextHop with D2
              //if yes, choose 1 and 2
              uint8_t bitmapS = bitmap1 & bitmap2;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr2;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);
                } else { //if not, check whether 2 and 3 share path
                    bitmapS = bitmap2 & bitmap3;
                    if(bitmapS != 0) {
                      int sharedPort = 0;
                      while(bitmapS != 0){
                        bitmapS = bitmapS >> 1;
                        sharedPort++;
                      }

                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      dataPacket1->k_value = 2;
                      dataPacket1->destinationAddr1 = destinationAddr2;
                      dataPacket1->destinationAddr2 = destinationAddr3;
                      dataPacket1->destinationAddr3 = 0;

                      output(sharedPort).push(p1);

                    } else {
                      //no share path, split
                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());
                      WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                      memcpy(p2->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                      dataPacket1->k_value = 1;
                      dataPacket2->k_value = 1;

                      dataPacket1->destinationAddr1 = destinationAddr2;
                      dataPacket1->destinationAddr2 = 0;
                      dataPacket1->destinationAddr3 = 0;
                      dataPacket2->destinationAddr1 = destinationAddr3;
                      dataPacket2->destinationAddr2 = 0;
                      dataPacket2->destinationAddr3 = 0;

                      output(forwardingPortSet2.front()).push(p1);
                      output(forwardingPortSet3.front()).push(p2);

                    }

                  }
            }

          } else {
            if(cost3 > cost1) {
              //choose 1 and 2
              //check whether share same port
                uint8_t bitmapS = bitmap1 & bitmap2;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr2;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);

                } else {
                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());
                  WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                  memcpy(p2->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                  dataPacket1->k_value = 1;
                  dataPacket2->k_value = 1;

                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = 0;
                  dataPacket1->destinationAddr3 = 0;
                  dataPacket2->destinationAddr1 = destinationAddr2;
                  dataPacket2->destinationAddr2 = 0;
                  dataPacket2->destinationAddr3 = 0;

                  output(forwardingPortSet1.front()).push(p1);
                  output(forwardingPortSet2.front()).push(p2);

                }
            } else if(cost3 < cost1) {
                  //choose D3, and check whether D1 or D2 share nextHop with D3
                  //first check D1 and D3
                  uint8_t bitmapS = bitmap1 & bitmap3;
                  if((bitmap1 & bitmap3) != 0) {
                      //choose D1 and D3
                      int sharedPort = 0;
                      while(bitmapS != 0){
                        bitmapS = bitmapS >> 1;
                        sharedPort++;
                      }

                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      dataPacket1->k_value = 2;
                      dataPacket1->destinationAddr1 = destinationAddr1;
                      dataPacket1->destinationAddr2 = destinationAddr3;
                      dataPacket1->destinationAddr3 = 0;

                      output(sharedPort).push(p1);

                  }else if((bitmap2 & bitmap3) != 0){
                      //choose D2 and D3
                      uint8_t bitmapS = bitmap2 & bitmap3;
                      int sharedPort = 0;
                      while(bitmapS != 0){
                        bitmapS = bitmapS >> 1;
                        sharedPort++;
                      }

                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      dataPacket1->k_value = 2;
                      dataPacket1->destinationAddr1 = destinationAddr2;
                      dataPacket1->destinationAddr2 = destinationAddr3;
                      dataPacket1->destinationAddr3 = 0;

                      output(sharedPort).push(p1);

                  }else {
                    //choose D1 and D3, split
                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());
                      WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                      memcpy(p2->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                      dataPacket1->k_value = 1;
                      dataPacket2->k_value = 1;

                      dataPacket1->destinationAddr1 = destinationAddr1;
                      dataPacket1->destinationAddr2 = 0;
                      dataPacket1->destinationAddr3 = 0;
                      dataPacket2->destinationAddr1 = destinationAddr3;
                      dataPacket2->destinationAddr2 = 0;
                      dataPacket2->destinationAddr3 = 0;

                      output(forwardingPortSet1.front()).push(p1);
                      output(forwardingPortSet3.front()).push(p2); 

                  }

            }else{
              //cost1 == cost2 == cost3
              //check whether 2 of them share nextHop

                //if 1 and 2 share path , choose 1 and 2
                uint8_t bitmapS = bitmap1 & bitmap2;
                if(bitmapS != 0) {
                  int sharedPort = 0;
                  while(bitmapS != 0){
                    bitmapS = bitmapS >> 1;
                    sharedPort++;
                  }

                  WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                  memcpy(p1->data(), packet->data(),packet->length());

                  struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                  dataPacket1->k_value = 2;
                  dataPacket1->destinationAddr1 = destinationAddr1;
                  dataPacket1->destinationAddr2 = destinationAddr2;
                  dataPacket1->destinationAddr3 = 0;

                  output(sharedPort).push(p1);
                }else {
                    //1 and 2 share no path
                    //check whether 1 and 3 share path
                    bitmapS = bitmap1 & bitmap3;
                    if(bitmapS != 0) {
                      int sharedPort = 0;
                      while(bitmapS != 0){
                        bitmapS = bitmapS >> 1;
                        sharedPort++;
                      }

                      WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                      memcpy(p1->data(), packet->data(),packet->length());

                      struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                      dataPacket1->k_value = 2;
                      dataPacket1->destinationAddr1 = destinationAddr1;
                      dataPacket1->destinationAddr2 = destinationAddr3;
                      dataPacket1->destinationAddr3 = 0;

                      output(sharedPort).push(p1);
                    } else {
                      // 1 and 3 share no path
                      //check whether 2 and 3 share path
                       bitmapS = bitmap2 & bitmap3;
                      if(bitmapS != 0) {
                        int sharedPort = 0;
                        while(bitmapS != 0){
                          bitmapS = bitmapS >> 1;
                          sharedPort++;
                        }

                        WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                        memcpy(p1->data(), packet->data(),packet->length());

                        struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                        dataPacket1->k_value = 2;
                        dataPacket1->destinationAddr1 = destinationAddr2;
                        dataPacket1->destinationAddr2 = destinationAddr3;
                        dataPacket1->destinationAddr3 = 0;

                        output(sharedPort).push(p1);
                      } else {
                          //2 and 3 share no path
                          //choose 1 and 2
                          //check whether share same port
                          bitmapS = bitmap1 & bitmap2;
                          if(bitmapS != 0) {
                            int sharedPort = 0;
                            while(bitmapS != 0){
                              bitmapS = bitmapS >> 1;
                              sharedPort++;
                            }

                            WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                            memcpy(p1->data(), packet->data(),packet->length());

                            struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                            dataPacket1->k_value = 2;
                            dataPacket1->destinationAddr1 = destinationAddr1;
                            dataPacket1->destinationAddr2 = destinationAddr2;
                            dataPacket1->destinationAddr3 = 0;

                            output(sharedPort).push(p1);

                          } else {
                            WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
                            memcpy(p1->data(), packet->data(),packet->length());
                            WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
                            memcpy(p2->data(), packet->data(),packet->length());

                            struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
                            struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();

                            dataPacket1->k_value = 1;
                            dataPacket2->k_value = 1;

                            dataPacket1->destinationAddr1 = destinationAddr1;
                            dataPacket1->destinationAddr2 = 0;
                            dataPacket1->destinationAddr3 = 0;
                            dataPacket2->destinationAddr1 = destinationAddr2;
                            dataPacket2->destinationAddr2 = 0;
                            dataPacket2->destinationAddr3 = 0;

                            output(forwardingPortSet1.front()).push(p1);
                            output(forwardingPortSet2.front()).push(p2);

                          }
                      } 

                    }
                } 

            }
          }

      }
      
      



      packet->kill();
  }

  if(dataPacket->k_value == 3) {
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

      if(bitmapS != 0){
          //three destinations share path
          int sharedPort = 0;
          while(bitmapS != 0){
            
            //if(bitmapS & 1 != 0) {
            /*

            store all shared port and then calculate cost

            multiple ports may be shared by all 3 detinations

            need to calculate the total cost, then choose the next port with minimum total cost

            TO BE DONE  
              
            because using this->routingTable->routingTable.get(dataPacket->destinationAddr1) can only return the first entry
            even if there multiple entry for destinationAddr1
            */

            /*now assume just share one port*/
            //}

            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          output(sharedPort).push(packet);

      }else if((bitmap1 & bitmap2)!= 0){
          //only 1 and 2 share path
          bitmapS = bitmap1 & bitmap2;
          int sharedPort = 0;
          while( bitmapS != 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }

          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
          struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
          dataPacket1->k_value = 2;
          dataPacket1->destinationAddr1 = dataPacket->destinationAddr1;
          dataPacket1->destinationAddr2 = dataPacket->destinationAddr2;
          dataPacket1->destinationAddr3 = 0;

          dataPacket2->k_value = 1;
          dataPacket2->destinationAddr1 = dataPacket->destinationAddr3;
          dataPacket2->destinationAddr2 = 0;
          dataPacket2->destinationAddr3 = 0;
          output(sharedPort).push(p1);
          output(forwardingPortSet3.front()).push(p2);

      }else if((bitmap1 & bitmap3) != 0) {
          // only 1 and 3 share path
          bitmapS = bitmap1 & bitmap3;
          int sharedPort = 0;
          while( bitmapS != 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());

          struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
          struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
          dataPacket1->k_value = 2;
          dataPacket2->k_value = 1;
          dataPacket1->destinationAddr1 = dataPacket->destinationAddr1;
          dataPacket1->destinationAddr2 = dataPacket->destinationAddr3;
          dataPacket1->destinationAddr3 = 0;
          dataPacket2->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket2->destinationAddr2 = 0;
          dataPacket2->destinationAddr3 = 0;
          output(sharedPort).push(p1);
          output(forwardingPortSet2.front()).push(p2);

      }else if ((bitmap2 & bitmap3) != 0) {
          // only 2 and 3 share path
          bitmapS = bitmap2 & bitmap3;
          int sharedPort = 0;
          while( bitmapS != 0){
            bitmapS = bitmapS >> 1;
            sharedPort++;
          }
          WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
          memcpy(p1->data(), packet->data(),packet->length());

          WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
          memcpy(p2->data(), packet->data(),packet->length());
          struct DataPacket *dataPacket1 = (struct DataPacket*) p1->data();
          struct DataPacket *dataPacket2 = (struct DataPacket*) p2->data();
          dataPacket1->k_value = 1;
          dataPacket2->k_value = 2;
          dataPacket1->destinationAddr1 = dataPacket->destinationAddr1;
          dataPacket1->destinationAddr2 = 0;
          dataPacket1->destinationAddr3 = 0;
          dataPacket2->destinationAddr1 = dataPacket->destinationAddr2;
          dataPacket2->destinationAddr2 = dataPacket->destinationAddr3;
          dataPacket1->destinationAddr3 = 0;
          output(forwardingPortSet1.front()).push(p1);
          output(sharedPort).push(p2);

      }else {  
          //no share path
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

         /*set destinationAddr2 and destinationAddr3 to be 0*/
         dataPacket1->destinationAddr1 = destinationAddr1;
         dataPacket1->destinationAddr2 = 0;
         dataPacket1->destinationAddr3 = 0;
         dataPacket2->destinationAddr1 = destinationAddr2;
         dataPacket2->destinationAddr2 = 0;
         dataPacket2->destinationAddr3 = 0;
         dataPacket3->destinationAddr1 = destinationAddr3;
         dataPacket3->destinationAddr2 = 0;
         dataPacket3->destinationAddr3 = 0;

         output(forwardingPortSet1.front()).push(p1);
         output(forwardingPortSet2.front()).push(p2);
         output(forwardingPortSet3.front()).push(p3);

      }

      packet->kill();
  }



}


CLICK_ENDDECLS 
EXPORT_ELEMENT(DataModule)

