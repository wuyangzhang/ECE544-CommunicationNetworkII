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
#ifndef MF_ASSOCTABLE_HH
#define MF_ASSOCTABLE_HH

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/timestamp.hh>

#include "mf.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

#define ASSOC_TIMEOUT_SEC 10
typedef struct {
  uint32_t assoc_guid;
  uint32_t host_guid; 
  Timestamp ts; 
} assoc_table_record_t; 


typedef HashTable<uint32_t, HashTable<uint32_t, assoc_table_record_t>* > AssocTable; 

class MF_AssocTable : public Element {
public:
  MF_AssocTable();
  ~MF_AssocTable();

  const char* class_name() const { return "MF_AssocTable"; };
  const char* port_count() const { return PORTS_0_0; };
  const char* processing() const { return AGNOSTIC; };
  int configure (Vector<String> &, ErrorHandler *);
  
  bool insert (uint32_t guid, uint32_t assoc_guid);
  int32_t get (uint32_t assoc_guid, Vector<uint32_t> &guids);
  bool remove (uint32_t guid, uint32_t deassoc_guid);
  int getAllClientGUIDs(Vector<uint32_t> &guids); 
  void print(); 
private:
  void clean(); 
  AssocTable *_assocTable;
  MF_Logger logger; 
}; 


CLICK_ENDDECLS
#endif
