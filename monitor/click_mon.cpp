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
 * Click monitor with OML backend for state persistence
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <sstream>
#include <fstream>
#include <sys/time.h>

#include <math.h>
#include <unistd.h>

#include <oml2/omlc.h>
#include "config.h"

#include "packet_stats.h"
#include "routing_stats.h"
#include "link_stats.h"

#define MON_PERIOD_SECS 1

using namespace std;


void print_usage(){

	printf("./click-mon <click-control-port> <self-id> --oml-config <oml-config-file>\n");
}

int main(int argc, const char *argv[]) {

	if (argc < 5) {
		print_usage();
                exit(0);
	}

	if (omlc_init("click_mon", &argc, argv, NULL) == -1) {
		cout << "FATAL: " << "Unable to init OML client" << endl;
		exit(1);
	}

	//register OML measurement points
	packet_stats pkt_stats(argv[2]);
	routing_stats rtg_stats(argv[2]);
	link_stats lnk_stats(argv[2]);

	if (omlc_start() == -1) {
		cout << "FATAL: " << "Unable start OML stream" << endl;
		exit(1);
	}

	/* connect to local click's control socket */
	sockaddr_in address;
	int result;
	int sockfd, len;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int port = atoi(argv[1]);
	address.sin_port = htons(port);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	len = sizeof(address);
	result = connect(sockfd, (sockaddr *)&address, len);
	if (result==-1) {
		perror("click-mon");
		exit(1);
	}
	char buf[100];
	/* ignore initial message from click */
	recv(sockfd, buf, 100, 0);

	unsigned mp_index = 0;
	for (;;mp_index++) {
		pkt_stats.read_from_click(sockfd);	
		rtg_stats.read_from_click(sockfd);	
		lnk_stats.read_from_click(sockfd);	

		pkt_stats.inject_into_oml(mp_index);
		rtg_stats.inject_into_oml(mp_index);
		lnk_stats.inject_into_oml(mp_index);
		sleep(MON_PERIOD_SECS);
	}
	omlc_close();

	return(0);
}
