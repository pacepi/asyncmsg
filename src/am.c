/*
 * am.c : main processdure, deal with asyncmsg structures
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
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "am.h"
#include "am_log.h"
#include "nanomsg/nn.h"
#include "nanomsg/pubsub.h"
#include "iniparser/iniparser.h"


static char input_conf[256] = "../conf/input.ini";
static char output_conf[256] = "../conf/output.ini";
static int if_daemon = 0;
LIST_HEAD(am_list);
static pthread_mutex_t am_mutex = PTHREAD_MUTEX_INITIALIZER;


#define BUF_SIZE (64*1024)


/*
 * all the asyncmsg structures are linked in am_list
 */
struct list_head *am_get_list()
{
    return &am_list;
}


/*
 * setup input sock, we use this to bind input address.
 * input sock works as SUB type.
 */
static int __am_setup_input_sock(asyncmsg_t *am)
{
    int ret = 0;
    am->input.sock = nn_socket (AF_SP, NN_SUB);
    if (am->input.sock < 0) {
        am_log_fatal("nn_socket fail\n");
    }

    ret = nn_setsockopt (am->input.sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    if (ret < 0) {
        am_log_fatal("nn_setsockopt NN_SUB_SUBSCRIBE fail\n");
    }

    ret = nn_bind(am->input.sock, am->input.addr);
    if (ret < 0) {
        am_log_fatal("nn_bind %s fail\n", am->input.addr);
    }

    return ret;
}


#ifdef INPROC
/*
 * setup transfer sock as inproc type.
 * transfer sock works as PUB type, publish data to backends
 * which are intersted in this kind of message.
 */
static int __am_setup_transfer_sock(asyncmsg_t *am)
{
    int ret = 0;
    char inproc[128] = {0};

    am->transfer_sock = nn_socket (AF_SP, NN_PUB);
    if (am->transfer_sock < 0) {
        am_log_fatal("nn_socket fail\n");
    }

    memset(inproc, 0x00, sizeof(inproc));
    snprintf(inproc, sizeof(inproc), "inproc://%s", am->input.section);
    ret = nn_bind(am->transfer_sock, inproc);
    if (ret < 0) {
        am_log_fatal("nn_bind %s fail\n", inproc);
    }

    return ret;
}
#endif


/*
 * remove unused input nodes, and remove all output nodes which has subscribed
 * this kind of section.
 */
int __am_input_remove_all_unused_nodes(const char *conf)
{
    asyncmsg_t *am = NULL, *pos = NULL;
    dictionary *ini = NULL;
    char key[128] = {0};
    char *address = NULL;

    if (list_empty(am_get_list()))
        return 0;

    ini = iniparser_load(conf);
    if (ini == NULL)
        am_log_warn("iniparser_load %s fail\n", conf);

    list_for_each_entry_safe(am, pos, am_get_list(), list) {
        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:address", am->input.section);
        address = iniparser_getstring(ini, key, NULL);
        /* original input miss ...*/
        if ((address == NULL) || (strcmp(address, am->input.addr))) {
            am_log_info("input %s : %s miss...\n", am->input.section, am->input.addr);
#ifdef INPROC
            nn_close(am->transfer_sock);
#endif
            /* close input sock must befor kill routine*/
            /* nn_recv has hold semphore, and nn_close will hang forever */
            nn_close(am->input.sock);
            am->input.sock = -1;

            /* stop __am_input_routine */
            pthread_cancel(am->thread);
            pthread_join(am->thread, NULL);

            /* remove all output nodes */
            am_output_remove_all_nodes(am);
            if (!list_empty(&am->output_list) || am->output_nr)
                am_log_warn("input %s : %s has output nodes yet...\n", am->input.section, am->input.addr);

            free(am->data);
            am->data = NULL;
            free(am->input.section);
            am->input.section = NULL;
            free(am->input.addr);
            am->input.addr = NULL;
            pthread_mutex_lock(&am_mutex);
            list_del(&am->list);
            pthread_mutex_unlock(&am_mutex);
            free(am);
            am = NULL;
        }
    }

    iniparser_freedict(ini);

    return 0;
}


/*
 * input routine :
 * 1,recive data from one or more PUB clients
 * 2,use a inproc sock, send recived data to one or more SUB group
 * 3,only one client of a SUB group can recive data via PUSH-PULL
 */
static void *__am_input_routine(void *arg)
{
    asyncmsg_t *am = (asyncmsg_t*)arg;
    int ret = 0;
    am_log_info("[tid = %lu]am input routine 0x%lx start...\n", syscall(SYS_gettid), am->thread);

    __am_setup_input_sock(am);
#ifdef INPROC
    __am_setup_transfer_sock(am);
#endif

    ret = 0;
    am->data = (char *)calloc(sizeof(char), BUF_SIZE);
    while (1) {
        /*always clear data*/
        if (unlikely(am == NULL) || unlikely(am->data == NULL)) {
            am_log_warn("am maybe released\n");
            break;
        }
        memset(am->data, 0x00, ret);

        /*TODO how to check data is complete*/
        ret = nn_recv(am->input.sock, am->data, BUF_SIZE, 0);
        if (unlikely(ret == -1)) {
            am_log_warn("am [%s : %s]nn_recv ret : %d, errno = %d\n", am->input.section, am->input.addr, ret, errno);
            if (errno == EBADF)
                break;
            else if (errno == EINTR) {
                ret = BUF_SIZE;
                continue;
            }
        }

        /*why check twice ?*/
        /*nn_recv maybe block for a long time, and this is a multi-thread case*/
        /*take care !!!*/
        if (unlikely(am != NULL) || unlikely(am->data != NULL)) {
            /* am_log_info("nn_recv : %s\n", am->data); */
#ifdef INPROC
            ret = nn_send(am->transfer_sock, am->data, ret, 0);
#else
            ret = am_output_send_msg(am, am->data, ret, NN_DONTWAIT);
#endif
        }
    }

    am_log_trace();
    return NULL;
}


/*
 * alloc a new am structure, and insert into am_list, setup asyncmsg
 */
asyncmsg_t *am_add_new(char *sec, char *addr)
{
    asyncmsg_t *am = (asyncmsg_t*)calloc(1, sizeof(asyncmsg_t));
    if (am == NULL)
        am_log_fatal("malloc asyncmsg_t fail\n");

    INIT_LIST_HEAD(&am->list);
    INIT_LIST_HEAD(&am->output_list);

    am->input.section = strdup(sec);
    am->input.addr = strdup(addr);
    if (am->input.addr == NULL)
        am_log_fatal("strdup input addr fail\n");

    pthread_mutex_init(&am->mutex, NULL);

    /*link all the asyncmsg into a list*/
    pthread_mutex_lock(&am_mutex);
    list_add_tail(&am->list, am_get_list());
    pthread_mutex_unlock(&am_mutex);

    /*each asyncmsg works in a thread*/
    pthread_create(&am->thread, NULL, __am_input_routine, am);

    return am;
}


/*
 * find asyncmsg entry, return NULL if there is no such entry
 */
asyncmsg_t *am_find(char *sec, char *addr)
{
    asyncmsg_t *am = NULL, *pos = NULL;

    list_for_each_entry(pos, am_get_list(), list) {
        if ((strcmp(addr, pos->input.addr) == 0) && (strcmp(sec, pos->input.section) == 0)) {
            am = pos;
            break;
        }
    }

    return am;
}


/*
 * find asyncmsg enter only by section name.
 * return NULL if there is no such entry
 */
asyncmsg_t *am_find_by_sec(char *sec)
{
    asyncmsg_t *am = NULL, *pos = NULL;

    list_for_each_entry(pos, am_get_list(), list) {
        if (strcmp(sec, pos->input.section) == 0) {
            am = pos;
            break;
        }
    }

    return am;
}


static void usage()
{
    printf("usage\n");
    printf("    -i : input config\n");
    printf("    -o : output config\n");
    printf("    -d : run as daemon\n");
    printf("    -h : show help information\n");
    exit(0);
}


static void init_cmdline(int argc, char *argv[])
{

    int cmd_opt = 0;

    while ((cmd_opt  =  getopt(argc, argv, "i:o:dh")) != EOF) {
        switch (cmd_opt) {
        case 'i' :
            if (optarg)
                strncpy(input_conf, optarg, sizeof(input_conf));
            else
                usage();

            break;

        case 'o' :
            if (optarg)
                strncpy(output_conf, optarg, sizeof(output_conf));
            else
                usage();

            break;

        case 'd' :
            if_daemon = 1;

            break;

        case 'h' :
        default :
            usage();
            break;
        }
    }

    am_log_info("input confif : %s\n", input_conf);
    am_log_info("output confif : %s\n", output_conf);
    am_log_info("run as daemon : %d\n", if_daemon);
}


/*
 * user can use SIGUSR1 to reload input config
 * and use SIGUSR2 to reload output confi
 */
static void __reload_sig_handler(int sig)
{
    if (sig == SIGUSR1) {
        /* 1, remove all unused nodes if any input miss
         * 2, add new input
         * 3, check output for new input
         */
        __am_input_remove_all_unused_nodes(input_conf);
        am_input_load_conf(input_conf);
        am_output_load_conf(output_conf);
    }
    else if (sig == SIGUSR2)
        am_output_load_conf(output_conf);
}


/*
 * user can use SIGCHLD to show debug message
 */
static void __debug_sig_handler(int sig)
{
    asyncmsg_t *am = NULL;
    list_for_each_entry(am, am_get_list(), list) {
        am_log_short("asyncmsg :\n");
        am_log_short("\tinput section : %s\n", am->input.section);
        am_log_short("\tinput address : %s\n", am->input.addr);
        am_log_short("\tinput socket  : %d\n", am->input.sock);
#ifdef INPROC
        am_log_short("\ttransfer sock : %d\n", am->transfer_sock);
#endif
        am_log_short("\toutput nr     : %d\n", am->output_nr);
        am_output_t *out = NULL;
        list_for_each_entry(out, &am->output_list, list) {
            am_log_short("\t\toutput address : %s\n", out->addr);
            am_log_short("\t\toutput socket  : %d\n", out->sock);
#ifdef INPROC
            am_log_short("\t\ttransfer sock  : %d\n", out->transfer_sock);
#endif
        }
    }
}


void init_sighandler()
{
    int i = 0;
    int32_t ign_sigs[] = {SIGPIPE};
    int32_t reload_sigs[] = {SIGUSR1, SIGUSR2};

    for (i = 0; i < sizeof(ign_sigs)/sizeof(ign_sigs[0]); i++)
        if (signal(ign_sigs[i], SIG_IGN) == SIG_ERR)
            am_log_error("init_sighandler %d error\n", ign_sigs[i]);

    for (i = 0; i < sizeof(reload_sigs)/sizeof(reload_sigs[0]); i++)
        if (signal(reload_sigs[i], __reload_sig_handler) == SIG_ERR)
            am_log_error("init_sighandler %d error\n", reload_sigs[i]);

    signal(SIGCHLD, __debug_sig_handler);
}


int main(int argc, char *argv[])
{
    init_cmdline(argc, argv);
    init_sighandler();
    am_input_load_conf(input_conf);
    am_output_load_conf(output_conf);

    while (1)
        sched_yield();

    return 0;
}
