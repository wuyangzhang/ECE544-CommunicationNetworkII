#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "packet.hh"
#include "MyRouter.hh"
CLICK_DECLS 
MyRouter::MyRouter() : _timer(this){}
MyRouter::~MyRouter(){}

int PacketClient::initialize(ErrorHandler *errh){
    if(_timer.initialize(this) == false){
        return errh->error("timer initiate failure!");
    }
    _timer.schedule_now();
    return 0;
}

void PacketClient::run_timer(Timer *timer) {

    assert(timer == &_timer);
    this->helloMessage();
    /* waiting for 2s, periodically send hello message.*/  
    _timer.reschedule_after_msec(2000);
}

void MyRouter::push(int port, Packet *p) {
    /* read the packet be pushed into the router, classify into: hello message, forwarding*/
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    int packetType = format->type;
    String in_addr = format->srcAddr;
    String out_addr = format->destAddr;
    if(packetType == 2){
        /*this is a hello message, the router updates the routing table*/
        this->updateForwardingTable(port, in_addr);
        p->kill();
    }

    if(packetType == 0){
        /*router should forwarding the packet */
        int forwardingPort = this->lookUpForwardingTable();
        if(forwardingPort == -1){
            click_chatter("Can not find forwarding port for this destination!");
            p->kill();
        }
        this->forwardingPacket(p, forwardingPort);
    }
}

int MyRouter::lookUpForwardingTable(String destAddr){
    int matchPort = -1;
    for(HashTable<int, String>::iterator it == this->forwardingTable.begin(); it; ++it){
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
            /* do nothing */
        else
            this->forwardingTable.replace(port, in_addr);
    }

}
void PacketClient::helloMessage(){
    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct PacketHeader)+5, 0);
    memset(helloPacket->data(), 0, helloPacket->length());
    struct PacketHeader *format = (struct PacketHeader*) helloPacket->data();
    format->type = 3;
    output(0).push(helloPacket);
    click_chatter("Sending out Hello Message");
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(MyRouter)