#ifndef CLICK_MFpacketshow_HH
#define CLICK_MFpacketshow_HH
#include <click/element.hh>
CLICK_DECLS

/*
 * =c
 * SetCRC32()
 * =s crc
 * calculates CRC32 and prepends to packet
 * =d
 * Computes a CRC32 over each packet and appends the 4 CRC
 * bytes to the packet.
 * =a CheckCRC32
 */

class EtherAddress;

class MFpacketshow : public Element { public:

  MFpacketshow();
  ~MFpacketshow();

  const char *class_name() const	{ return "MFpacketshow"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return AGNOSTIC; }

  Packet *simple_action(Packet *);

};

CLICK_ENDDECLS
#endif
