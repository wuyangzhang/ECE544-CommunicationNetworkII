/**
 * Copyright (c) 2013, Rutgers, The State University of New Jersey
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization(s) stated above nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef NET_ADDR_H
#define NET_ADDR_H

#include <sstream>
#include <iostream>
#include <string>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

#include "gnrscommon.hh"

using namespace std;

//address types
#define NET_ADDR_TYPE_IPV4_PORT 0
#define NET_ADDR_TYPE_GUID 1
#define NET_ADDR_TYPE_NA 2

#define NET_ADDR_LEN_IPV4_PORT 6
#define NET_ADDR_LEN_GUID 20
//TODO currently binary encoded with GUID length
#define NET_ADDR_LEN_NA 20

#define PORT_OFFSET_IPV4_PORT 4

#define MAX_NET_ADDR_ENCODED_LEN 20

class NetAddr {

    public:

        uint16_t type;

        /* encoded length */
        uint16_t len;
        /* 
         * encoding here is determined by address type 
         * refer to documentation for considered address
         * types and suggested encodings
         */
        string value;

        unsigned char bytes[MAX_NET_ADDR_ENCODED_LEN];

        /* absolute time in milliseconds since 1970 epoch after which
         * the address, when associated with a GUID, should be 
         * considered invalid. Ignored if 0 - no defined expiry
         */
        uint32_t expiry;

        /* relative time in milliseconds following a caching operation after
         * which the address, when associated with a GUID,  should 
         * be considered invalid. Ignored if 0 - no defined ttl
         */
        uint32_t ttl;


        //these should go into derived class
        string hostname_or_ip;
        int port;

        //these should go into derived class
        string netaddr_net;
        uint32_t netaddr_local;

        //these should go into derived class
        //GUID netaddr type
        uint32_t guid_int;

        NetAddr(uint16_t addr_type, string addr_value, 
                uint32_t expiry_ms = 0, uint32_t ttl_ms = 0): 
                type(addr_type), value(addr_value), 
                expiry(expiry_ms), ttl(ttl_ms) {
                
            switch(type) {
                case NET_ADDR_TYPE_IPV4_PORT: 
                    len = NET_ADDR_LEN_IPV4_PORT; 
                    parse_ip_and_port();
                    break;
                case NET_ADDR_TYPE_GUID: 
                    len = NET_ADDR_LEN_GUID; 
                    /* the guid is currently encoded as a 4-byte integer */
                    parse_guid();
                    break;
                case NET_ADDR_TYPE_NA: 
                    len = NET_ADDR_LEN_NA; 
                    //Currently, binary encoding assumes that the
                    //addr is encoded in 20 bytes. Upto first 15 bytes network
                    //addr, and next 4 bytes the local addr. Local address
                    //at present is specified as 32 bit integer, while
                    //network addr is uninterpreted bytes of upto 15 bytes
                    // 16th byte is null for string ops
                    type = NET_ADDR_TYPE_GUID;
                    parse_netaddr();
                    break;
            }
        }
     
        /* constructor for binary encoded char array for address value */
        NetAddr(uint16_t addr_type, unsigned char* buf, int len): 
                                        type(addr_type), len(len) { 
            char tmp[32];
            switch(type) {
                case NET_ADDR_TYPE_IPV4_PORT: 
                    if(len != NET_ADDR_LEN_IPV4_PORT){
                        cerr << "ERROR: NetAddr: length mismatch for type: " 
                            << addr_type << "; expect " 
                            << NET_ADDR_LEN_IPV4_PORT 
                            << "got " << len << endl;	
                        //TODO throw exception
                        return;
                    }
                    sprintf(tmp, "%u.%u.%u.%u", *(uint8_t*)&buf[0], 
                                                *(uint8_t*)&buf[1], 
                                                *(uint8_t*)&buf[2], 
                                                *(uint8_t*)&buf[3]);
                    hostname_or_ip.assign(tmp); 
                    port = ntohs(*((uint16_t*)&buf[PORT_OFFSET_IPV4_PORT]));
                    sprintf(tmp, "%s:%u", hostname_or_ip.c_str(), port); 
                    value.assign(tmp);
                    break;
            
                case NET_ADDR_TYPE_GUID: 
                    /* the guid is currently encoded as a 4-byte integer */
                    guid_int  = ntohl(*(uint32_t*)&buf[16]);
                    sprintf(tmp, "%u", guid_int); 
                    value.assign(tmp);
                    break;

                case NET_ADDR_TYPE_NA: 
                    //Currently, binary encoding assumes that the
                    //addr is encoded in 20 bytes. Upto first 15 bytes network
                    //addr, and next 4 bytes the local addr. Local address
                    //at present is a 32 bit integer, while
                    //network addr is uninterpreted bytes of upto 15 bytes
                    // 16th byte is null for string ops

                    netaddr_net.assign((const char*)buf);
                    netaddr_local = ntohl(*(uint32_t*)&buf[16]);
                    sprintf(tmp, "%s:%u", netaddr_net.c_str(), netaddr_local); 
                    value.assign(tmp);
                    break;
            }
            memcpy(bytes, buf, len);
        }

        //IPV4/TCP/UDP specfic functions
        string get_hostname_or_ip() {
            return hostname_or_ip;
        }

        int get_port() {
            return port;
        }

        addr_tlv_t get_tlv() {
            addr_tlv_t t;
            t.type = type;
            t.len = len;
            t.value = bytes;

            return t;
        }

    private:
        void parse_ip_and_port(){

            //format: (hostname_or_IP:port)
            const char* s = value.c_str(); 
            const char* index = strchr(s, ':');
            hostname_or_ip = value.substr(0, index - s);
            struct hostent *he;
            if (!(he = gethostbyname(hostname_or_ip.c_str()))) {
                //TODO throw an exception
                cerr << "FATAL: gethostbyname failed for host: " 
                    << hostname_or_ip
                    << endl;
                exit(EXIT_FAILURE);
            }

            memcpy(bytes, he->h_addr_list[0], he->h_length);
            //port
            istringstream strm(value.substr(index - s + 1, value.length()));
            strm >> port;
	        *(uint16_t*)&bytes[he->h_length] = htons(port);
        }

        /* the guid is currently encoded as a 4-byte integer */
        void parse_guid() {
            
            istringstream strm(value);
            strm >> guid_int;
            *(uint32_t*)&bytes[16] = htonl(guid_int);
        }

        void parse_netaddr(){

            //format: net:local
            const char* s = value.c_str(); 
            const char* index = strchr(s, ':');

            //network part of NA
            memset(bytes, 0, 16); 
            if((index - s) < 16){
                memcpy(bytes, s, index - s);
                netaddr_net = value.substr(0, index - s);
            }else{
                memcpy(bytes, s, 15);
                netaddr_net = value.substr(0, 15);
            }

            //local part of NA
            istringstream strm_lcl(value.substr(index - s + 1, value.length()));
            strm_lcl >> netaddr_local;
	        *(uint32_t*)&bytes[16] = htonl(netaddr_local);
        }
};

#endif //NET_ADDR_H
