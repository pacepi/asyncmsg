/*
   Copyright (c) 2012 Martin Sustrik  All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation __FILE__s (the "Software"),
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

#include <sys/types.h>
#include <unistd.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "../target/inc/nanomsg/nn.h"
#include "../target/inc/nanomsg/pubsub.h"
/*
#include "../src/nn.h"
#include "../src/pubsub.h"
*/


#define SOCKET_ADDRESS "tcp://127.0.0.1:5565"

void test_recv (int sock, char *data)
{
	size_t data_len;
	int rc;
	char *buf;

	data_len = strlen (data);
	/*  We allocate plus one byte so that we are sure that message received
		has correct length and not truncated  */
	buf = malloc (data_len+1);
	memset(buf, 0x00, data_len + 1);

	rc = nn_recv (sock, buf, data_len+1, 0);
	if (rc < 0) {
		fprintf (stderr, "Failed to recv: %s [%d] (%s:%d)\n",
				nn_err_strerror (errno), errno, __FILE__, __LINE__);
		nn_err_abort ();
	}

	printf("pid = %d, data = %s\n", getpid(), buf);

	/*
	if (rc != (int)data_len) {
		fprintf (stderr, "Received data has wrong length: %d != %d (%s:%d)\n",
				rc, (int) data_len,
				__FILE__, __LINE__);
		nn_err_abort ();
	}

	if (memcmp (data, buf, data_len) != 0) {
		fprintf (stderr, "Received data is wrong (%s:%d)\n", __FILE__, __LINE__);
		nn_err_abort ();
	}
	*/

	free (buf);
}


int main ()
{
	int rc;
	int sub;
	char buf [8];
	size_t sz;

	for (rc = 0; rc < 4; rc++)
	{
		if (fork() == 0)
		{
			memset(buf, 0x00, sizeof(buf));
			if (getpid() % 2)
				strcpy(buf, "abcd");
			else
				strcpy(buf, "0123");
			break;
		}
	}

	sub = nn_socket (AF_SP, NN_SUB);
	if (sub == -1) {
		fprintf (stderr, "Failed create socket: %s [%d] (%s:%d)\n",
				nn_err_strerror (errno), errno, __FILE__, __LINE__);
		nn_err_abort ();
	}

	rc = nn_setsockopt (sub, NN_SUB, NN_SUB_SUBSCRIBE, buf, strlen(buf));
	sz = sizeof (buf);
	rc = nn_getsockopt (sub, NN_SUB, NN_SUB_SUBSCRIBE, buf, &sz);

    rc = nn_connect (sub, SOCKET_ADDRESS);
    if(rc < 0) {
        fprintf (stderr, "Failed connect to \"%s\": %s [%d] (%s:%d)\n",
            SOCKET_ADDRESS,
            nn_err_strerror (errno), errno, __FILE__, __LINE__);
        nn_err_abort ();
    }

	/*  Wait till connections are established to prevent message loss. */
	//nn_sleep (10);
	//sleep (5);

	while (1)
		test_recv (sub, "0123456789012345678901234567890123456789");

	rc = nn_close (sub);
	if ((rc != 0) && (errno != EBADF && errno != ETERM)) {
		fprintf (stderr, "Failed to close socket: %s [%d] (%s:%d)\n",
				nn_err_strerror (errno), errno, __FILE__, __LINE__);
		nn_err_abort ();
	}


	return 0;
}

