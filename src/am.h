#ifndef __AM_H__
#define __AM_H__


/*
 * am.h : main data structure and fuctions are defined here
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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


typedef struct am_input_s {
    char *section;	/*section name*/
    char *addr;	/*input address*/
    int sock;	/*input sock*/
} am_input_t;


typedef struct am_output_s {
    struct list_head list;	/*link all the output struct in asyncmsg_s::output_list*/
    char *addr;
#ifdef INPROC
    pthread_t thread;
    int transfer_sock;	/*transfer sock, work as SUB type*/
#endif
    int sock;	/*output sock, bind as PUSH type*/
    char *data;		/*private data*/
} am_output_t;


typedef struct asyncmsg_s {
    struct list_head list;	/*link all am struct in a list*/
    pthread_t thread;	/*work thread*/
    am_input_t input;	/* input sock, bind as SUB type*/
#ifdef INPROC
    int transfer_sock;	/*transfer sock, bind as PUB type*/
#endif
    int output_nr;	/* number of real output sock*/
    struct list_head output_list;	/*output sock list, run as SUB type*/
    pthread_mutex_t mutex;	/*for output_list*/
    char *data;		/*private data*/
} asyncmsg_t;


struct list_head *am_get_list();
asyncmsg_t *am_add_new(char *sec, char *addr);
asyncmsg_t *am_find(char *sec, char *addr);
asyncmsg_t *am_find_by_sec(char *sec);
int am_input_load_conf(const char *conf);
int am_input_permission(const char *conf);
int am_output_load_conf(const char *conf);
int am_output_permission(const char *conf);
int am_output_remove_all_nodes(asyncmsg_t *am);
int am_output_send_msg(asyncmsg_t *am, const void *data, int size, int flag);


#ifdef __cplusplus
}
#endif

#endif	//__AM_H__
