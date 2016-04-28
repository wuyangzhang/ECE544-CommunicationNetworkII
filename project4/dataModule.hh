/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_DATAMODULE_HH 
#define CLICK_DATAMODULE_HH 
  
#include <click/element.hh>
  
#include "routingTable.hh"

CLICK_DECLS

class DataModule : public Element {
  public:
    DataModule();
    ~DataModule();

    const char *class_name() const { return "DataModule";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }

    void push(int port, Packet *packet);
    int initialize(ErrorHandler* errh);
    int configure(Vector<String>&, ErrorHandler*);

    RoutingTable* routingTable;

    Vector<uint16_t> sortCost(RoutingTable* routingTable , uint16_t , uint16_t , uint16_t );
    int checkSharedPort(uint16_t, uint16_t );
    int checkSharedPort(uint16_t, uint16_t, uint16_t);
    void sendPacket1(uint16_t, Packet* packet);
    void sendPacket1(uint16_t, uint16_t, int, Packet* packet);
    void sendPacket2(uint16_t, uint16_t, uint16_t, int, Packet* );





		
}; 

CLICK_ENDDECLS
#endif 
