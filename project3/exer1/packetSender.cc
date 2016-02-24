/***********************************************************************
* packetSender.cc
*   
* stop and wait for ack
*  
 
*      Author: Wuyang Zhang
************************************************************************/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "packet.hh"
#include "packetSender.hh"
CLICK_DECLS 

PacketSender::PacketSender() : _timer(this) {
	sequenceNumber = 1;
}

PacketSender::~PacketSender(){
	
}

int PacketSender::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void PacketSender::run_timer(Timer *timer) {

    assert(timer == &_timer);

    /*if get the ack, send the next sequence, otherwise resend the current one*/  
    if(ackSequenceTable.get(sequenceNumber) == true){
      this->sequenceNumber++;
    }
  
    WritablePacket *packet = Packet::make(0,0,sizeof(struct PacketHeader)+5, 0);
    memset(packet->data(),0,packet->length());
    struct PacketHeader *format = (struct PacketHeader*) packet->data();
    format->type = 0;
    format->sequenceNumber = this->sequenceNumber;
    char *data = (char*)(packet->data()+sizeof(struct PacketHeader));
    memcpy(data, "hello", 5);
    output(0).push(packet);

    /* start awaiting for the ack for the packet */
    click_chatter("Awaiting for the ack of sequence %d", sequenceNumber);

  
    /* waiting for 1s, check whether receive ack message.*/  
    _timer.reschedule_after_msec(500);
}

void PacketSender::push(int, Packet *p){
    struct PacketHeader *header = (struct PacketHeader*) p->data();
    uint8_t type = header->type;
    uint32_t seq = header->sequenceNumber;
    click_chatter("Receiving an ack message sequence %d", seq);
    /*if type = 1, receive an ack message*/
    if (type == 1)
        if(seq == sequenceNumber)
                ackSequenceTable.set(seq,true);
} 
CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketSender)
