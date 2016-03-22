#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "packet.hh"

CLICK_DECLS 
PacketReceiver::PacketReceiver(){}
PacketReceiver::~PacketReceiver(){}

void PacketReceiver::push(int, Packet *p) {
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    //TO DO can we change the content of the receiving packets directly?
    /* revise type from 0 -> 1, sending out the ack message for a given sequenceNumber*/
    format-> type = 1;
    char* packetContent = char* (p->data() + sizeof(struct PacketHeader));
    memcpy(packetContent, "ack", 3);
    output(0).push(p);
    click_chatter("Sending out the ack for message sequence %d", format->sequenceNumber);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketReceiver)
