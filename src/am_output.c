/*
 * am_output.c : process output address
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include "am.h"
#include "am_log.h"
#include "iniparser/iniparser.h"
#include "utils/list.h"
#include "nanomsg/nn.h"
#include "nanomsg/pubsub.h"
#include "nanomsg/pipeline.h"


#define BUF_SIZE (64*1024)


#ifdef INPROC
/*
 * setup transfer sock, we use this to connect inproc.
 * transfer sock works as SUB type.
 */
static int __am_output_transfer_sock(asyncmsg_t *am, am_output_t *out)
{
    int ret = 0;
    char inproc[128] = {0};

    out->transfer_sock = nn_socket (AF_SP, NN_SUB);
    if (out->transfer_sock < 0) {
        am_log_fatal("nn_socket fail\n");
    }

    ret = nn_setsockopt (out->transfer_sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    if (ret < 0) {
        am_log_fatal("nn_setsockopt NN_SUB_SUBSCRIBE fail\n");
    }

    int linger = 10;
    ret = nn_setsockopt (out->transfer_sock, NN_SOL_SOCKET, NN_LINGER, &linger, sizeof (linger));
    if (ret < 0) {
        am_log_fatal("nn_setsockopt NN_LINGER fail\n");
    }

    memset(inproc, 0x00, sizeof(inproc));
    snprintf(inproc, sizeof(inproc), "inproc://%s", am->input.section);
    ret = nn_connect(out->transfer_sock, inproc);
    if (ret < 0) {
        am_log_fatal("nn_connect %s fail\n", am->input.addr);
    }

    return ret;
}
#endif


/*
 * setup real output sock.
 * outpu sock works as PUSH type, push data to client.
 * only one client of a cluster could recive data.
 */
static int __am_output_sock(am_output_t *out)
{
    int ret = 0;

    out->sock = nn_socket (AF_SP, NN_PUSH);
    if (out->sock < 0) {
        am_log_fatal("nn_socket fail\n");
    }

    int linger = 10;
    ret = nn_setsockopt (out->sock, NN_SOL_SOCKET, NN_LINGER, &linger, sizeof (linger));
    if (ret < 0) {
        am_log_fatal("nn_setsockopt NN_LINGER fail\n");
    }

    ret = nn_bind(out->sock, out->addr);
    if (ret < 0) {
        am_log_fatal("nn_bind %s fail\n", out->addr);
    }

    return ret;
}


#ifdef INPROC
/*
 * output work routine :
 * 1, use transfer sock to recive data
 * 2, use output sock to send data
 */
static void *__am_output_routine(void *arg)
{
    am_output_t *out = (am_output_t *)arg;
    int ret = 0;
    am_log_info("[tid = %lu]am output routine 0x%lx start...\n", syscall(SYS_gettid), out->thread);

    out->data = (char *)calloc(sizeof(char), BUF_SIZE);
    while (1) {
        if ((out == NULL) || (out->data == NULL)) {
            am_log_warn("am_output maybe released\n");
            break;
        }

        /*always clear data*/
        memset(out->data, 0x00, ret);
        /*TODO how to check data is complete*/
        if (out->transfer_sock >= 0)
            ret = nn_recv(out->transfer_sock, out->data, BUF_SIZE, 0);
        else
            break;

        if ((ret == -1) && (errno == EBADF)) {
            am_log_warn("nn_recv ret : %d, errno = %d\n", ret, errno);
            break;
        }

        if (ret < 0) {
            ret = BUF_SIZE;
            continue;
        }

        if ((out != NULL) && (out->data != NULL)) {
            am_log_info("nn_recv : %s\n", out->data);
            if (out->sock >= 0)
                ret = nn_send(out->sock, out->data, ret, 0);
        }

        if ((ret < 0) || (ret > BUF_SIZE)) {
            ret = BUF_SIZE;
            continue;
        }
    }

    return NULL;
}
#endif


/*
 * if output node has been linked in output list ?
 */
am_output_t *__am_output_find_node(asyncmsg_t *am, const char *addr)
{
    am_output_t *out = NULL, *pos = NULL;

    list_for_each_entry(pos, &am->output_list, list) {
        if (strcmp(pos->addr, addr) == 0) {
            out = pos;
            break;
        }
    }

    return out;
}


/*
 * add a new output node
 */
am_output_t *__am_output_new_node(asyncmsg_t *am, const char *addr)
{
    am_output_t *out = NULL;

    out = (am_output_t*)calloc(1, sizeof(am_output_t));
    if (out == NULL)
        am_log_fatal("calloc am_output_t fail\n");

    out->addr = strdup(addr);
#ifdef INPROC
    __am_output_transfer_sock(am, out);
#endif
    __am_output_sock(out);
    pthread_mutex_lock(&am->mutex);
    list_add_tail(&out->list, &am->output_list);
    am->output_nr++;
    pthread_mutex_unlock(&am->mutex);
#ifdef INPROC
    pthread_create(&out->thread, NULL, __am_output_routine, out);
#endif

    am_log_info("section : %s has new output node : %s\n", am->input.addr, addr);
    return out;
}


/*
 * output node has alread been built ?
 */
int __am_output_setup_node(asyncmsg_t *am, const char *addr)
{
    am_output_t *out = __am_output_find_node(am, addr);
    if (out == NULL) {
        __am_output_new_node(am, addr);
    } else {
        am_log_info("section : %s output node : %s has already inserted\n", am->input.addr, addr);
    }

    return 0;
}


/*
 * remove all output nodes which are linked in am
 */
int am_output_remove_all_nodes(asyncmsg_t *am)
{
    am_output_t *out = NULL, *pos = NULL;

    if (list_empty(&am->output_list)) {
        am_log_info("asyncmsg : %s has no output list\n", am->input.section);
        return 0;
    }

    list_for_each_entry_safe(out, pos, &am->output_list, list) {
        /*free all output infomation, take care about sequence*/
        /*may need mutex*/

        nn_close(out->sock);
        out->sock = -1;
#ifdef INPROC
        nn_close(out->transfer_sock);
        out->transfer_sock = -1;
        pthread_cancel(out->thread);
        pthread_join(out->thread, NULL);
#endif
        free(out->addr);
        out->addr = NULL;
        asm volatile ("" : : : "memory");
        free(out->data);
        out->data = NULL;
        pthread_mutex_lock(&am->mutex);
        list_del(&out->list);
        pthread_mutex_unlock(&am->mutex);
        free(out);
        am->output_nr--;
    }

    return 0;
}


/*
 * send data to all of the output nodes
 */
int am_output_send_msg(asyncmsg_t *am, const void *data, int size, int flag)
{
    am_output_t *out = NULL;
    int ret = 0;

    if (unlikely(am->input.sock < 0)) {
        am_log_warn("am section %s is releasing...\n", am->input.section);
        return 0;
    }

    pthread_mutex_lock(&am->mutex);
    list_for_each_entry(out, &am->output_list, list) {
        if (unlikely(out->sock < 0)) {
            am_log_warn("am output sock invalid\n");
            continue;
        }

        /*because of NN_DONTWAIT flag, nn_send may fail if output has no client*/
        ret = nn_send(out->sock, data, size, flag);
        if (ret != size)
        {
            /* TODO may report error here */
            /*am_log_warn("am output %s nn_send fail, ret : %d\n", out->addr, ret);*/
            continue;
        }
    }
    pthread_mutex_unlock(&am->mutex);

    return 0;
}



/*
 * remove unused output nodes which are linked in am
 */
int am_output_remove_unused_nodes(asyncmsg_t *am, const char *conf)
{
    dictionary *ini = NULL;
    int nsec = 0;
    int i = 0;
    char *sec = NULL;
    char key[128] = {0};
    char *address = NULL;
    am_output_t *out = NULL, *pos = NULL;

    if (list_empty(&am->output_list)) {
        am_log_info("asyncmsg : %s has no output list\n", am->input.section);
        return 0;
    }

    ini = iniparser_load(conf);
    if (ini == NULL)
        am_log_warn("iniparser_load %s fail\n", conf);

    list_for_each_entry_safe(out, pos, &am->output_list, list) {
        am_output_t *notfound = out;
        /*parse all sections, find output has been removed or not*/
        nsec = iniparser_getnsec(ini);
        for (i = 0; i < nsec; i++) {
            sec = iniparser_getsecname(ini, i);
            if (sec == NULL)
                am_log_fatal("section %d invalid\n", i);

            memset(key, 0x00, sizeof(key));
            snprintf(key, sizeof(key), "%s:address", sec);
            address = iniparser_getstring(ini, key, "");
            /*hit*/
            if (strcmp(address, out->addr) == 0) {
                notfound = NULL;
                address = NULL;
                am_log_info("find output node : %s in output config\n", out->addr);
                break;
            }
        }

        if (notfound != NULL) {
            /*free all output infomation, take care about sequence*/
            am_log_info("not find output node : %s in output config", out->addr);
            nn_close(notfound->sock);
            notfound->sock = -1;
#ifdef INPROC
            nn_close(notfound->transfer_sock);
            notfound->transfer_sock = -1;
            pthread_cancel(notfound->thread);
            pthread_join(out->thread, NULL);
#endif
            free(notfound->addr);
            notfound->addr = NULL;
            asm volatile ("" : : : "memory");
            free(notfound->data);
            notfound->data = NULL;
            pthread_mutex_lock(&am->mutex);
            list_del(&notfound->list);
            pthread_mutex_unlock(&am->mutex);
            free(notfound);
            am->output_nr--;
        }
    }

    iniparser_freedict(ini);

    return 0;
}


/*
 * parse all am, and remove all unused output nodes
 */
int __am_output_remove_all_unused_nodes(const char *conf)
{
    asyncmsg_t *pos = NULL;
    list_for_each_entry(pos, am_get_list(), list) {
        am_output_remove_unused_nodes(pos, conf);
    }

    return 0;
}


/*
 * load output config file
 * inster new output node, and remove unused output node
 */
int am_output_load_conf(const char *conf)
{
    dictionary *ini = NULL;
    int nsec = 0;
    int i = 0;
    char *sec = NULL;
    char key[128] = {0};
    char *subscribe = NULL;
    char *address = NULL;

    am_log_info("conf : %s\n", conf);
    ini = iniparser_load(conf);
    if (ini == NULL)
        am_log_warn("iniparser_load %s fail\n", conf);

    nsec = iniparser_getnsec(ini);
    if (nsec == 0)
        am_log_warn("conf %s has no section\n", conf);

    __am_output_remove_all_unused_nodes(conf);

    /*parse all sections in output config*/
    for (i = 0; i < nsec; i++) {
        sec = iniparser_getsecname(ini, i);
        if (sec == NULL)
            am_log_fatal("section %d invalid\n", i);

        /*current section has subscribe message or not*/
        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:subscribe", sec);
        subscribe = iniparser_getstring(ini, key, NULL);
        if (subscribe == NULL) {
            am_log_warn("key %s has no subscribe\n", key);
            continue;
        }

        /*am_list has this kind of message ?*/
        asyncmsg_t *am = am_find_by_sec(subscribe);
        if (am == NULL) {
            am_log_warn("no such PUB message : %s \n", subscribe);
            continue;
        }

        /*current section has address or not*/
        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:address", sec);
        address = iniparser_getstring(ini, key, NULL);
        if (address == NULL) {
            am_log_warn("key %s has no address to bind\n", key);
            continue;
        }

        /*ok, all infomation checked, insert output node into asyncmsg structure*/
        __am_output_setup_node(am, address);
    }

    if (list_empty(am_get_list()))
        am_log_warn("am_list empty\n");

    iniparser_freedict(ini);

    return 0;
}
