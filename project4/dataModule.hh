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


		
}; 

CLICK_ENDDECLS
#endif 
