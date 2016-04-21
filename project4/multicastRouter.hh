
#ifndef CLICK_MULTICASTROUTER_HH 
#define CLICK_MULTICASTROUTER_HH  

/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/string.hh>

#include <packet.hh>
CLICK_DECLS

class MulticastRouter : public Element {
    public:
        MulticastRouter();
        ~MulticastRouter();
        const char *class_name() const { return "MulticastRouter";}
        const char *port_count() const { return "1-/1-";} 
        const char *processing() const { return PUSH; }
	    void run_timer(Timer*);
        int initialize(ErrorHandler*);
        int configure(Vector<String>&, ErrorHandler*);
        void push(const int, Packet *p);

        /* ------------------------------------------------------------
         * Key funcitons of router events : 
         * send/receive hello, 
         * send/receive update, 
         * receive data, 
         * send/receive ack
         * ------------------------------------------------------------
         */
        void sendHello();
        void receiveHello(const int port, Packet *p);

        void sendUpdate();
        void receiveUpdate(const int port, Packet *p);

        void sendAck(const int port, const uint8_t sequenceNumber, const uint16_t destinationAddr);
        void receiveAck();

        void receiveData();

        void updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop);
        void updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port);

        void computeRoutingTable();
        void computeForwardingTable();

        int lookUpForwardingTable(uint32_t destAddr);
        void forwardingPacket(Packet *p, int port);

        void printRoutingTable();
        void printForwardingTable();
    private: 
        Timer _timer;

        /* ------------------------------------------------------------
         *   forwarding table format
         *   Destination | Cost | Port
         * ------------------------------------------------------------
         *   routing table format
         *   Destination | Cost | Next Hop
         *
         * ------------------------------------------------------------
         */
        struct updateInfo{
            uint16_t sourceAddr;
            uint32_t cost,
            uint16_t nextHop
        };

        struct routingTableParam{
            uint32_t cost,
            uint16_t nextHop
        };
        HashTable<uint16_t, routingTableParam>routingTable;

        struct forwardTableParam{
            uint32_t cost,
            uint8_t port
        };
        HashTable<uint16_t, forwardTableParam> forwardingTable;

        uint32_t srcAddr;
        uint32_t destAddr;
        uint32_t helloSequence;
        uint32_t updateSequence;
}; 

CLICK_ENDDECLS
#endif 
