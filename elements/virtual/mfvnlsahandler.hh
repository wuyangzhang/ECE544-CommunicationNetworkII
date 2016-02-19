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
#ifndef MF_VN_LSA_HANDLER_HH
#define MF_VN_LSA_HANDLER_HH
#define DEFAULT_CHK_SIZE 7000

#include <click/element.hh>
#include <click/timer.hh>
#include "mf.hh"
#include "mflogger.hh"
#include "mfvnroutingtable.hh"
#include "mfvnneighbortable.hh"
#include "mfvirtualctrlmsg.hh"
#include "mfchunkmanager.hh"
#include "gnrsrespcache.hh"
#include "mfvninfocontainer.hh"
#include "mfvnmanager.hh"

CLICK_DECLS

/**
 * Virtual Link-State Advertisements (LSAs) Handler - Creates and forwards Application specific routing metric information 
 * 
 * Helps implement a virtual link state routing protocol by periodically 
 * distributing local node's observations on virtual link state with it's
 * neighbors and also to record and forward (re-broadcast) any virtual LSAs
 * heard from it's virtual neighbors.
 * 
 * Message Type on Ports:
 * Input:  	port 0: Virtual LSA from neighbor
 *
 * Output: 	port 0: Virtual LSA (own or neighbor) for broadcast
 *
 * Timer: To periodically construct & broadcast virtual LSA of local node
 */ 

//TODO refactoring: move to use new vn container. Schedule LSA transmission for each VN

class MF_VNLSAHandler: public Element{
public:
	MF_VNLSAHandler();
	~MF_VNLSAHandler();

	const char *class_name() const	{ return "MF_VNLSAHandler"; }
	const char *port_count() const	{ return "1/1-2"; }
	const char *processing() const	{ return PUSH; }

	void push (int, Packet *);
	int configure(Vector<String> &, ErrorHandler *);
	int initialize(ErrorHandler *);
	static void callback(Timer *t, void *user_data);
	void run_timer(Timer *t, void *user_data);

	void addVNTimer(uint32_t vnGUID);
	void removeVNTimer(uint32_t vnGUID);

private:
	uint32_t _service_id;
	uint32_t _pkt_size;
	uint32_t _chk_size;
	uint32_t my_guid;
	MF_VNManager * _vnManager;
	MF_ChunkManager *_chunkManager;


	typedef struct timerInfo {
		MF_VNLSAHandler *handler;
		uint32_t guid;
		Timer *timer;
	}timerInfo_t;
	typedef HashTable<uint32_t, timerInfo_t*> timerMap_t;

	timerMap_t timerMap;

	/* functions */
	void handleVirtualLSA(Chunk * chunk);
	void handleVirtualASP(Chunk * chunk);

	MF_Logger logger;
};

CLICK_ENDDECLS;

#endif //VIRTUALLSAHANDLER_HH
