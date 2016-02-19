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
#ifndef CONTROL_SOCKET_H
#define CONTROL_SOCKET_H

/*
 *  Created on: July 25, 2011
 *      Author: sk
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <sstream>
#include <fstream>
#include <sys/time.h>

using namespace std;

class control_socket{

public:
	enum {
		RCVD_CHK_COUNT_QUERY,
		SENT_CHK_COUNT_QUERY,
		IN_PKT_COUNT_QUERY,
		OUT_PKT_COUNT_QUERY,
		DROPPED_PKT_COUNT_QUERY,
		IN_BYTE_COUNT_QUERY,
		OUT_BYTE_COUNT_QUERY,
		IN_BIT_RATE_QUERY,
		OUT_BIT_RATE_QUERY,
		SETT_QUERY,
		LETT_QUERY,
		FORWARD_FLAG_QUERY,
		NEIGHBOR_TABLE_QUERY,
		RCVD_CHK_BYTES_QUERY,
		PORT_TRAFFIC_TABLE_QUERY
	};

	static int sendCommand(int socket_fd, const char *cmd);
	static void receiveData(int socket_fd, char *buffer, int buf_length, 									int query);
};

#endif //CONTROL_SOCKET_H
