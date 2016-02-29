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
#include "control_socket.h"

int control_socket::sendCommand(int socket_fd, const char *cmd) {
	int byte_send;
	int length = strlen(cmd);
	if((byte_send = send(socket_fd, cmd, length, 0))<0) {
		perror("didn't write");
		exit(1);
	}
	//cout << "send " << byte_send << " bytes" << endl;

	return byte_send;
}

/* returns response to query as c-string in the passed buffer */
void control_socket::receiveData(int socket_fd, char *buffer, int buf_length, int query) {
	string type;

	//TODO check if request/responses can be batch processed and if
	//query arg can be used to discern what to return here. The
	//challenge then is to handle out of order responses....

	int byte_recvd;
	if((byte_recvd = recv(socket_fd, buffer, buf_length, 0))<0) {
		perror("didn't receive");
		exit(1);
	}
	//cout << "received " << byte_recvd << " bytes" << endl;
	string raw_data(buffer, byte_recvd);
	stringstream ss(stringstream::in | stringstream::out);
	char temp[100];
	ss << raw_data;
	for(int i=0; i<3; i++) { // each piece of data contains 3 lines, the last line has the data
		ss.getline(temp, 100);
		if(temp[0]=='D' && temp[1]=='A') break;
	}
	ss.getline(buffer, 100);
}
