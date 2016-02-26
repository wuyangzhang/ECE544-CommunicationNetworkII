/*
	Author: WuyangZhang
	Creating Date: Feb 23/ 2016
	Define simple agnostic element
*/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "simpleagnosticelement.hh"

CLICK_DECLS
SimpleAgnosticElement::SimpleAgnosticElement(){}
SimpleAgnosticElement::~SimpleAgnosticElement(){}

int SimpleAgnosticElement::configure(Vector<String> &conf, ErrorHandler *errh){
	/*
		check valid packet size
	*/
	if (cp_va_kparse(conf, this, errh, "MAXPACKETSIZE", cpkM, cpInteger, &maxSize, cpEnd) < 0) return -1;
	if (maxSize <= 0) return errh->error("maxsize should be larger than 0");
	return 0;
}


void SimpleAgnosticElement::push(int, Packet *p){
	click_chatter("Receiving a packet size %d", p->length());
	/*
		push packet out
	*/
	if(p->length() > maxSize)
		p->kill();
	else
		output(0).push(p);
}

Packet* SimpleAgnosticElement::pull(int){
	Packet *p = input(0).pull();
	if(p == 0)return 0;
	click_chatter("Receiving a packet size %d", p->length());

	if(p->length() > maxSize)
		p->kill();
	else
		return p;


}
CLICK_ENDDECLS
EXPORT_ELEMENT(SimpleAgnosticElement)