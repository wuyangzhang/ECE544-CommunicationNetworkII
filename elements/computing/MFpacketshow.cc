/*
 * MFpacketshow.{cc, hh} -- show information of a received packet
 * 
 * 
 * 
 * setcrc32.{cc,hh} -- element sets CRC 32 checksum
 * Robert Morris
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/crc32.h>

#include <click/straccum.hh>

#include "MFpacketshow.hh"
#include "click_MF.hh"
#include "mf.hh"

#if CLICK_USERLEVEL
# include <stdio.h>
#endif

CLICK_DECLS

MFpacketshow::MFpacketshow()
{
}

MFpacketshow::~MFpacketshow()
{
}

Packet *
MFpacketshow::simple_action(Packet *p)
{
  //routing_header *MF_routing_header;
  StringAccum sa(1000); 
  //  routing_header *header = (routing_header *) (p->data());  
  uint32_t *SRV_ID = (uint32_t *) (p->data());
  
  int srv;
  //sa << ntohl(header->service_ID);
  int len = p->length();
  //unsigned int crc = 0xffffffff;
  //crc = update_crc(crc, (char *) p->data(), len); // calculate the CRC

  //WritablePacket *q = p; //Add space for data after the packet.
  
  // start from p->data
  //printf("%l", p->data)
  //memcpy(q->data() + len, &crc, 4);
  srv = ntohl(*SRV_ID);
  
 
  switch (srv)
  {
    case DATA_PKT: sa<< "DATA";
      break;
    case CSYN_PKT: sa<< "CSYN";
      break;
	case CSYN_ACK_PKT: sa<< "CSYN_ACK";
      break;
	case LP_PKT: sa<< "LP";
      break;
	case LP_ACK_PKT: sa<< "LP_ACK";
      break;
	case LSA_PKT: sa<< "LSA";
      break;
	case ASSOC_PKT: sa<< "ASSOC";
      break;
	case DASSOC_PKT: sa<< "DASSOC";
      break;
	default: sa<< "unknown";
      break;
  }  
  
  //sa<<" Src: "<<ntohl(header->src_GUID)<<" Dst: "<<ntohl(header->dst_GUID)<<" DstNA: "<<header->dst_NA;
  click_chatter("%s", sa.c_str());
  return(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MFpacketshow)
ELEMENT_MT_SAFE(MFpacketshow)
