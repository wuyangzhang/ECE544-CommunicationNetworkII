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
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    int packetType = format->type;
    this->srcAddr = format->srcAddr;
    this->destAddr = format->destAddr;
    click_chatter("[Router] receiving a pakcet from type%d", format->type);
    click_chatter("[Router]receiving a packet from %d to %d", format->srcAddr, format->destAddr);
    if(packetType == 2){
        /*this is a hello message, the router updates the routing table*/
        click_chatter("[Router] update forwarding table, port %d", port );
        this->updateForwardingTable(port, this->srcAddr);
        p->kill();
    }

    if(packetType == 0 || packetType == 1){
        /*router should forwarding the packet */
        int forwardingPort = this->lookUpForwardingTable(this->destAddr);
        if(forwardingPort == -1){
            click_chatter("Can not find forwarding port for this destination!");
            p->kill();
	        return ;
        }
        /* if find forwarding port */
        this->forwardingPacket(p, forwardingPort);
        click_chatter("[Router] forwarding packet to port %d", forwardingPort);
    }
}

int MyRouter::lookUpForwardingTable(uint32_t destAddr){
    int matchPort = -1;
    for(HashTable<int, String>::iterator it = this->forwardingTable.begin(); it; ++it){
      if(String::compare(it.value(), destAddr)==0)
            matchPort = it.key();
    }
    return matchPort;
}
void MyRouter::forwardingPacket(Packet *p, int port){
    output(port).push(p);
    click_chatter("[Router] forwarding packet to port %d", port);
}

void MyRouter::updateForwardingTable(int port, uint32_t in_addr){
    /* hashtable. port -> MAC_Addr */
    if(!this->forwardingTable[port]) /*port is not in the table*/
        this->forwardingTable.set(port, in_addr);
    else {
      if(String::compare(this->forwardingTable.get(port),in_addr) == 0)
            /* do nothing */;
        else
            this->forwardingTable.set(port, in_addr);
    }

}


CLICK_ENDDECLS 
EXPORT_ELEMENT(MyRouter)
