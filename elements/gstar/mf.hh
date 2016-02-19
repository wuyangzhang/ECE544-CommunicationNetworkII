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
 * MF.hh
 * This file includes all the packet format used in MF data processing
 *  Created on: Jun 2, 2011
 *      Author: sk
 */

#ifndef CLICK_MF_HH_
#define CLICK_MF_HH_

#include "mfguid.hh"
#include "mfgstarctrlmsg.hh"
#include "mfhopmsg.hh"
#include "mfheader.hh"
#include "mfroutingheader.hh"
#include "mftransportheader.hh"
#include "mfextheader.hh"
#include "mfmhomeextheader.hh"

CLICK_DECLS

// defaule config
// default eth packet size if unspecified 
#define DEFAULT_PKT_SIZE 1400
// defaule window size for sending if unspecified
#define DEFAULT_WINDOW_SIZE 1

//Control 
#define BROADCAST_GUID 0 

typedef GUID GUID_t;

#define MAX_NUM_NA 10

/* timeouts */
//TODO: relative values for the following could depend on several
//factors, including degree of mobility, link/path reliability, 
//load balancing concerns...

#define REBIND_NA_TIMEOUT_SEC 30
#define REBIND_NXTHOP_TIMEOUT_SEC 10

/* Annotations offsets */
#define SRC_GUID_ANNO_OFFSET 0 /* 4 byte anno */
#define NXTHOP_ANNO_OFFSET 4 /* 4 byte anno */

/* 1-byte annos, bytes 40-47 */
#define IN_PORT_ANNO_OFFSET 40 /* 1 byte anno */
#define OUT_PORT_ANNO_OFFSET 41 /* 1 byte anno */

typedef int seconds_t;

enum {
  GNRS_LOOKUP,
  GNRS_INSERT,
  GNRS_UPDATE
};

typedef RoutingHeader routing_header;

/* All below are Click internal message formats */

typedef struct {
  uint32_t sid;              //service id of the chunk
  uint32_t chunk_id;         //every received chunk has an unique chunk id
  uint32_t upper_protocol;   //upper layer protocol for classifier
} chk_internal_trans_t;  


/* Internal message to signal a client association to the 
 * GNRS service module */

struct gnrs_req{
  uint8_t type;
  /* GUID of associating client */
  uint8_t GUID[GUID_LENGTH];
  /* client specified network addr. Could represent endpoint id of hosting
   * node */
  uint32_t net_addr;
  /* client specified weight for attachment point */
  uint16_t weight;
};
typedef struct gnrs_req gnrs_req_t; 

#define LOOKUP_MAX_NA 10
struct NA_entry{
  uint32_t net_addr;
  unsigned long TTL;
  uint16_t weight;
};
typedef NA_entry NA_entry_t; 

struct gnrs_lkup_resp{
  uint8_t GUID[GUID_LENGTH];
  uint16_t na_num;
  struct NA_entry NAs[LOOKUP_MAX_NA];
};
typedef struct gnrs_lkup_resp gnrs_lkup_resp_t; 

//bitrate protocol
enum {
  BITRATE_REQ,
  BITRATE_RESP
};

typedef struct {
  uint32_t type;
  uint32_t GUID;
} bitrate_req_t;

typedef struct {
  uint32_t type; 
  uint32_t GUID;
  uint32_t edge_router_GUID;
  double bitrate;
  uint32_t valid_time_sec;
} bitrate_resp_t;

//measurmemt 
//64 bytes
typedef struct {
  uint32_t num_to_measure;
} start_measurement_msg_t; 

CLICK_ENDDECLS

#endif /* CLICK_MF_HH_ */
