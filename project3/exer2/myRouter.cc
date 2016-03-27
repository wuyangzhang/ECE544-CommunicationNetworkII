#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "packet.hh"
#include "myRouter.hh"
CLICK_DECLS 
MyRouter::MyRouter() : _timer(this){}
MyRouter::~MyRouter(){}

void MyRouter::push(int port, Packet *p) {
    /* read the packet be pushed into the router, classify into: hello message, forwarding*/
    click_chatter("[Router] receive packet from port %d", port);
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    int packetType = format->type;
    String in_addr = format->srcAddr;
    String out_addr = format->destAddr;
    if(packetType == 2){
        /*this is a hello message, the router updates the routing table*/
        click_chatter("[Router] update forwarding table");
        this->updateForwardingTable(port, in_addr);
        p->kill();
    }

    if(packetType == 0){
        /*router should forwarding the packet */
        int forwardingPort = this->lookUpForwardingTable(out_addr);
        if(forwardingPort == -1){
            click_chatter("Can not find forwarding port for this destination!");
            p->kill();
        }
        this->forwardingPacket(p, forwardingPort);
        click_chatter("[Router] forwarding packet to port %d", forwardingPort);
    }
}

int MyRouter::lookUpForwardingTable(String destAddr){
    int matchPort = -1;
    for(HashTable<int, String>::iterator it = this->forwardingTable.begin(); it; ++it){
        if(it.value() == destAddr)
            matchPort = it.key();
    }
    return matchPort;
}
void MyRouter::forwardingPacket(Packet *p, int port){
    output(port).push(p);
    click_chatter("[Router] forwarding packet to port %d", port);
}

void MyRouter::updateForwardingTable(int port, String in_addr){
    /* hashtable. port -> MAC_Addr */
    if(!this->forwardingTable[port]) /*port is not in the table*/
        this->forwardingTable.set(port, in_addr);
    else {
        if(this->forwardingTable.get(port) == in_addr)
            /* do nothing */;
        else
            this->forwardingTable.replace(port, in_addr);
    }

}


CLICK_ENDDECLS 
EXPORT_ELEMENT(MyRouter)