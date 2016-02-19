/*
 *  Copyright (c) 2010-2013, Rutgers, The State University of New Jersey
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the organization(s) stated above nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * MF_ARPTable.hh
 *
 *  Created on: Jul 16, 2011
 *      Author: Kai Su
 */

#ifndef MF_ARPTABLE_HH_
#define MF_ARPTABLE_HH_

#include <click/element.hh>
#include <click/etheraddress.hh>
//#include <click/hashmap.hh>
#include <click/hashtable.hh>
#include <click/string.hh>

#include "mflogger.hh"

CLICK_DECLS

class MF_ARPTable : public Element {
 public:

  MF_ARPTable();
  ~MF_ARPTable();

  const char *class_name() const	{ return "MF_ARPTable"; }
  const char *port_count() const	{ return PORTS_0_0; }
  const char *processing() const	{ return AGNOSTIC; }
  int configure(Vector<String> &, ErrorHandler *);

  void setMAC(uint32_t, EtherAddress);
  EtherAddress getMAC(uint32_t);
  uint32_t getGUID(EtherAddress);
  void setPort(uint32_t, int);
  int getPort(uint32_t);
  void setIP(uint32_t, IPAddress);
  IPAddress getIP(uint32_t);

 private:
  HashTable<uint32_t, EtherAddress> *_ARPMap;
  HashTable<String, uint32_t> *_etherMap;
        /* ports to differentiate from which interface the neighbor
         * GUID is connected to router */
  HashTable<uint32_t, int> *_portMap; 
  HashTable<uint32_t, IPAddress> *_IPMap;

  MF_Logger logger;
};

CLICK_ENDDECLS


#endif /* MF_ARPTABLE_HH_ */
