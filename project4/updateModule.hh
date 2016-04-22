/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_UPDATEMODULE_HH 
#define CLICK_UPDATEMODULE_HH 
  
#include <click/element.hh>
#include <click/timer.hh>

#include "routingTable.hh"  
#include "ackModule.hh"

CLICK_DECLS

class UpdateModule : public Element {
  public:
    UpdateModule();
    ~UpdateModule();

    const char *class_name() const { return "UpdateModule";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }
    void run_timer(Timer*);
    int initialize(ErrorHandler*);
//    int initialize();

    int configure(Vector<String>&, ErrorHandler*);

    void push(int port, Packet *packet);

    void sendUpdate();
    void receiveUpdate(const int port, Packet *p);
    void sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr);


  private:
    AckModule* ackModule;
    RoutingTable* routingTable;
    Timer _timerUpdate;
    uint16_t _myAddr;
    uint32_t _delay;
    uint32_t _period;
    uint32_t _timeout;
    uint8_t updateSequence;

}; 

CLICK_ENDDECLS
#endif 
