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

CLICK_DECLS 

PacketSender::PacketSender() : _timer(this) {
	sequence = 1;
}

PacketSender::~PacketSender(){
	
}

int PacketSender::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void PacketSender::run_timer(Timer *timer) {
	//sequence++;
    assert(timer == &_timer);
    WritablePacket *packet = Packet::make(0,0,sizeof(struct PacketHeader)+5, 0);
    memset(packet->data(),0,packet->length());
    struct PacketHeader *format = (struct PacketHeader*) packet->data();
    format->type = sequence%2;
    format->size = sizeof(struct PacketHeader)+5;
	char *data = (char*)(packet->data()+sizeof(struct PacketHeader));
	memcpy(data, "hello", 5);
    output(0).push(packet);

    /* start awaiting for the ack for the packet */
    click_chatter("Awaiting for the ack of sequence %d", sequence);
    /* waiting for 1s, check whether receive ack message.*/
    usleep(1000*1000)
    /*if get the ack, send the next sequence, otherwise resend the current one*/
    if(ackSequenceTable.get(sequence) == true){
        sequence++;
    }
    _timer.reschedule_after_msec(0);
}

void PacketSender::receiveAck(Packet *p, uint32_t sequenceAwaitAck){
    struct PacketHeader *header = (struct PacketHeader*) packet->data();
    uint8_t type = header->type;
    uint32_t seq = header->sequence;
    /*if type = 1, receive an ack message*/
    if (type == 1)
        if(seq == sequenceAwaitAck)
                ackSequenceTable.set(seq,true);
} 
CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketSender)
