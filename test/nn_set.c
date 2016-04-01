/*
   Copyright (c) 2016 Zhenwei Pi  All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom
   the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "../target/inc/nanomsg/pubsub.h"
#include "../target/inc/nanomsg/nn.h"


char address[sizeof("255.255.255.255") + 1] = "127.0.0.1";
int port = 2000;
int total = 1;
int sleepms = 0;
int fail = 0;
struct timeval start, finish;


void show_result()
{
	printf("Result  :\n");
	printf("Address : %s\n", address);
	printf("Port    : %d\n", port);
	printf("Total   : %d\n", total);
	printf("Fail    : %d\n", fail);
	printf("Sussess : %d\n", total - fail);
	printf("Time    : %ld\n", (finish.tv_sec - start.tv_sec) * 1000 * 1000 + finish.tv_usec - start.tv_usec);
}


int test_send(int sock, char *data)
{
	size_t data_len;
	int rc;

	data_len = strlen (data);

	rc = nn_send (sock, data, data_len, 0);
	if (rc < 0) {
		perror("nn_send");
		fail++;
		return -1;
	}
	if (rc != (int)data_len) {
		fail++;
		return -1;
	}

	return rc;
}


void parse_cmdline(int argc, char *argv[])
{
	char opt = 0;
	while ((opt = getopt(argc, argv, "a:p:s:t:")) != -1) {
		switch (opt) {
			case 'a' :
				strcpy(address, optarg);
				break;

			case 'p' :
				port = atoi(optarg);
				break;

			case 's' :
				sleepms = atoi(optarg);
				break;

			case 't' :
				total = atoi(optarg);
				break;

			case 'h' :
			default: 
				printf("%s -a 127.0.0.1 -p 2000 -s 1[ms] -t 100\n", argv[0]);
				exit(-1);
		}
	}
}


int main (int argc, char *argv[])
{
	int rc;
	int pub;
	size_t sz;
	char buf[64] = {0};
	char tcp_addr[32] = {0};

	parse_cmdline(argc, argv);

	pub = nn_socket (AF_SP, NN_PUB);
	if (pub == -1) {
		perror("nn_socket");
		return -1;
	}

	memset(tcp_addr, 0x00, sizeof(tcp_addr));
	snprintf(tcp_addr, sizeof(tcp_addr), "tcp://%s:%d", address, port);
	rc = nn_connect (pub, tcp_addr);
	if(rc < 0) {
		perror("nn_connect");
		return -1;
	}

	sleep (2);	/*wait established*/
	int pid = getpid();
	gettimeofday(&start, NULL);
	for (rc = 0; rc < total; rc++)
	{
		snprintf(buf, sizeof(buf), "%d : MSG%d", pid, rc);
		test_send(pub, buf);
		if (sleepms)
			usleep (sleepms * 1000);
	}
	gettimeofday(&finish, NULL);

	show_result();

	rc = nn_close (pub);
	if ((rc != 0) && (errno != EBADF && errno != ETERM)) {
		perror("nn_close");
		return -1;
	}

	return 0;
}

