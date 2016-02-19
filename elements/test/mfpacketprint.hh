#ifndef MF_PACKETPRINT_HH
#define MF_PACKETPRINT_HH

#include <click/element.hh>
#include <clicknet/ether.h>

#include "mf.hh"
#include "mflogger.hh"

CLICK_DECLS

class MF_PacketPrint : public Element {
public:
  MF_PacketPrint();
  ~MF_PacketPrint(); 
  const char* class_name() const { return "MF_PacketPrint"; }
  const char* port_count() const { return "1/0"; }
  const char* procssing()  const { return "h/h"; }
  
  int configure(Vector<String>&, ErrorHandler *);
  void push(int port, Packet *p);
private:
  MF_Logger logger; 
}; 

CLICK_ENDDECLS

#endif
