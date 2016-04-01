#include "asyncmsg.h"
#include "nanomsg/nn.h"
#include "nanomsg/pubsub.h"
#include "nanomsg/pipeline.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct am_api_s {
    int sock;
    int type;
} am_api_t;


int asyncmsg_initialize(asyncmsg_t *am, const char *addr, const int port, const int type)
{
    int ret = 0;
    char tcp_addr[32] = {0};

    am_api_t *api = calloc(1, sizeof(am_api_t));
    if (api == NULL)
        return (errno = -ENOMEM);

    if (type == ASYNCMSG_GET)
        api->sock = nn_socket (AF_SP, NN_PULL);
    else if (type == ASYNCMSG_SET)
        api->sock = nn_socket (AF_SP, NN_PUB);
    if (api->sock < 0) {
        return -errno;
        goto release_api;
    }

    memset(tcp_addr, 0x00, sizeof(tcp_addr));
    snprintf(tcp_addr, sizeof(tcp_addr), "tcp://%s:%d", addr, port);
    ret = nn_connect(api->sock, tcp_addr);
    if (ret < 0) {
        return -errno;
        goto release_sock;
    }

    api->type = type;
    *am = api;

    return ret;

release_sock :
    nn_close(api->sock);

release_api :
    free(api);
    *am = NULL;

    return ret;
}


int asyncmsg_send(const asyncmsg_t am, const void *data, const int size, const int flag)
{
    am_api_t *api = (am_api_t *)am;
    int ret = 0;

    ret = nn_send(api->sock, data, size, flag);
    if (ret == size)
        return ret;
    else
        return -1;
}

int asyncmsg_recv(const asyncmsg_t am, void *data, const int size, const int flag)
{
    am_api_t *api = (am_api_t *)am;
    int ret = 0;

    ret = nn_recv(api->sock, data, size, flag);
    return ret;
}

int asyncmsg_finalize(const asyncmsg_t am)
{
    am_api_t *api = (am_api_t *)am;

    return nn_close(api->sock);
}


