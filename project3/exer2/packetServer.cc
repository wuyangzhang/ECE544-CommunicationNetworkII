#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "packet.hh"
#include "packetServer.hh"
CLICK_DECLS 
PacketServer::PacketServer() : _timer(this){}
PacketServer::~PacketServer(){}

int PacketServer::initialize(ErrorHandler *errh){
  _timer.initialize(this);
  _timer.schedule_now();
  return 0;
}

void PacketServer::run_timer(Timer *timer) {

    assert(timer == &_timer);
    this->helloMessage();
    /* waiting for 2s, periodically send hello message.*/  
    _timer.reschedule_after_msec(2000);
}

void PacketServer::push(int, Packet *p) {
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    /* revise type from 0 -> 1, sending out the ack message for a given sequenceNumber*/    
    format-> type = 1;
    /* exchange src and dest addr*/
    String in_addr = format->srcAddr;
    String out_addr = format->destAddr;
    
    format->srcAddr = out_addr;
    format->destAddr = in_addr;

    //    WritablePacket *ackPacket = Packet::clone(*p);
    char* packetContent = (char*) (p->data() + sizeof(struct PacketHeader));
    memcpy(packetContent, "ack", 3);
    output(0).push(p);
    click_chatter("Sending out the ack for message sequence %d", format->sequenceNumber);
}

void PacketServer::helloMessage(){
    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct PacketHeader)+5, 0);
    memset(helloPacket->data(), 0, helloPacket->length());
    struct PacketHeader *format = (struct PacketHeader*) helloPacket->data();
    format->type = 2;
    output(0).push(helloPacket);
    click_chatter("Sending out Hello Message");
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketServer)
