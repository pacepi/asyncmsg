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
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MEretHANTABILITY,
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
#include "asyncmsg/asyncmsg.h"


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
	printf("Time    : %ld[microseconds]\n", (finish.tv_sec - start.tv_sec) * 1000 * 1000 + finish.tv_usec - start.tv_usec);
}


int test_send(asyncmsg_t am, char *data)
{
	size_t data_len;
	int ret;

	data_len = strlen (data);

	ret = asyncmsg_send (am, data, data_len, 0);
	if (ret < 0) {
		perror("asyncmsg_send");
		fail++;
		return -1;
	}

	if (ret != (int)data_len) {
		fail++;
		return -1;
	}

	return ret;
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
	int ret;
	asyncmsg_t am;
	size_t sz;
	char buf[64] = {0};
	char tcp_addr[32] = {0};

	parse_cmdline(argc, argv);

	ret = asyncmsg_initialize(&am, address, port, ASYNCMSG_SET);

	sleep (2);	/*wait established*/
	int pid = getpid();
	gettimeofday(&start, NULL);
	for (ret = 0; ret < total; ret++)
	{
		snprintf(buf, sizeof(buf), "%d : MSG%d", pid, ret);
		test_send(am, buf);
		if (sleepms)
			usleep (sleepms * 1000);
	}
	gettimeofday(&finish, NULL);

	show_result();

	ret = asyncmsg_finalize (am);
	if ((ret != 0) && (errno != EBADF)) {
		perror("asyncmsg_finalize");
		return -1;
	}

	return 0;
}

