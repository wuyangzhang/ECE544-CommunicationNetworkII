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
#ifndef MF_ETHERDROP_HH_
#define MF_ETHERDROP_HH_

#include <click/element.hh>
#include <click/task.hh>
#include <click/vector.hh>
#include <click/hashmap.hh>
#include "mf.hh"
#include "mflogger.hh"
#include "mfarptable.hh"

CLICK_DECLS

class MF_EtherDrop : public Element {
public:
	MF_EtherDrop();
	~MF_EtherDrop();
	const char *class_name() const		{ return "MF_EtherDrop"; }
	const char *port_count() const		{ return "1/1"; }
	const char *processing() const		{ return "h/h"; }
    
	int configure(Vector<String>&, ErrorHandler *);
	void push(int port, Packet *p);

private:
	MF_ARPTable *_ARPTable;
	click_ether _ethh;    
	//uint8_t _mac_2nd[6];
	click_ether _ethh2;    

	MF_Logger logger;
};


CLICK_ENDDECLS
#endif /* MF_EtherDrop_HH_ */

