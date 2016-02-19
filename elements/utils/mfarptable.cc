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
/********************************************************************
 * MF_ARPTable.cc
 * This element provides the data structure for mapping between GUID
 * and ethernet address and the operations required to manipulate that.
 *  Created on: Jul 16, 2011
 *      Author: Kai Su
 ********************************************************************/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>


#include "mfarptable.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS
MF_ARPTable::MF_ARPTable(): logger(MF_Logger::init()){
  _ARPMap = new HashTable<uint32_t, EtherAddress>; 
  _etherMap = new HashTable<String, uint32_t>;
  _portMap = new HashTable<uint32_t, int>;
  _IPMap = new HashTable<uint32_t, IPAddress>;
}

MF_ARPTable::~MF_ARPTable() {
}

int MF_ARPTable::configure(Vector<String> &conf, ErrorHandler *errh) {

  if (cp_va_kparse(conf, this, errh, cpEnd) < 0) {
    return -1;
  }
  return 0;
}

void MF_ARPTable::setMAC(uint32_t GUID, EtherAddress ea) {

  _ARPMap->set(GUID, ea);
  String eth_str = ea.unparse();
  _etherMap->set(eth_str, GUID);
  logger.log(MF_TRACE, 
             "arp_tbl: Inserting GUID: %u mac %s", 
             GUID, eth_str.c_str());
}

EtherAddress MF_ARPTable::getMAC(uint32_t GUID) {

  return _ARPMap->get(GUID);
}

/* reverse lookup on MAC address */
uint32_t MF_ARPTable::getGUID(EtherAddress ea) {
  String eth_str = ea.unparse();
  uint32_t GUID = _etherMap->get(eth_str);
  logger.log(MF_TRACE, 
             "arp_tbl: Get GUID for mac %s, resp GUID: %u", 
             eth_str.c_str(), GUID);
  return GUID;
}

void MF_ARPTable::setPort(uint32_t GUID, int port) {
  logger.log(MF_TRACE, 
             "arp_tbl: Inserting port: %d for GUID: %u", 
             port, GUID);
  //note +1 below to differntiate not found case
  _portMap->set(GUID, port + 1);
}

/* Returns 255 if entry not found. Specifically,
 * this will result in broadcast if return
 * value is used as tag in PaintSwitch
 */
int MF_ARPTable::getPort(uint32_t GUID) {

  int mod_port = _portMap->get(GUID);
  int port = 255;
  if (mod_port > 0) {
    port = mod_port - 1;
  }
  logger.log(MF_TRACE, 
             "arp_tbl: Returning i/f port: %d for GUID: %u", 
             port, GUID);
  return port;
}

void MF_ARPTable::setIP(uint32_t GUID, IPAddress ip) {

  _IPMap->set(GUID, ip);
  String ip_str = ip.unparse();
  logger.log(MF_TRACE, 
             "arp_tbl: setting GUID/IP: %u/%s mapping", 
             GUID, ip_str.c_str());
}

IPAddress MF_ARPTable::getIP(uint32_t GUID) {

  return _IPMap->get(GUID);
}
/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<uint32_t, EtherAddress>;
template class HashMap<String, uint32_t>;
template class HashMap<uint32_t, int>;
template class HashMap<uint32_t, IPAddress>;
#endif
*/
CLICK_ENDDECLS
EXPORT_ELEMENT(MF_ARPTable)
