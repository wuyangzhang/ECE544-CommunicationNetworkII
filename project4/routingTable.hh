/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_ROUTINGTABLE_HH 
#define CLICK_ROUTINGTABLE_HH 
  
#include <click/element.hh>
#include <click/timer.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>


CLICK_DECLS

class RoutingTable : public Element {
    public:
        RoutingTable();
        ~RoutingTable();

        const char *class_name() const { return "RoutingTable";}
        const char *port_count() const { return "0/0";}
        const char *processing() const { return PUSH; }
		
		void push(int port, Packet *packet);
        int initialize(ErrorHandler* errh);

        void updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop);
        void updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port);
        void updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop, const uint8_t sharedPath);
        void updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port, const uint8_t sharedPath);

        void computeRoutingTable(const uint16_t sourceAddr, const uint32_t cost, const uint16_t nextHop);
        void computeForwardingTable(const uint16_t sourceAddr, const uint32_t cost, const uint8_t port);

        int lookUpForwardingTable(uint16_t destAddr);

        void printRoutingTable();
        void printForwardingTable();

        /* ------------------------------------------------------------
         *   Forwarding table format:
         *
         *   Destination | Cost | Port
         * ------------------------------------------------------------
         *   
         *    Routing table format:
         *
         *    Destination | Cost | Next Hop
         *
         * ------------------------------------------------------------
         */
        struct updateInfo{
            uint16_t sourceAddr;
            uint32_t cost;
	        Vector<uint16_t> nextHop;
        };

       
        struct forwardingTableParam{
            uint32_t cost;
            uint8_t portCount; /* size of list port count */
            Vector<uint8_t> port; /* store multiple port */
        };

        HashTable<uint16_t, struct forwardingTableParam*> forwardingTable;

        struct routingTableParam{
            uint32_t cost;
            uint16_t hopCount; /* size of list nextHop */
            Vector<uint16_t> nextHop; /*store multiple next hop*/
        };


        HashTable<uint16_t, struct routingTableParam*> routingTable;


        uint32_t srcAddr;
        uint32_t destAddr;
        uint32_t helloSequence;
        uint32_t updateSequence;
		
}; 

CLICK_ENDDECLS
#endif 
