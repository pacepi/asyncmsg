#ifndef __ASYNCMSG_H__
#define __ASYNCMSG_H__


/*
 * asyncmsg.h : APIs
 * Copyright (c) 2016 Zhenwei Pi  All rights reserved.
 * using nanomsg APIs is OK !!!
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


#ifdef __cplusplus
extern "C" {
#endif


#define ASYNCMSG_GET 1
#define ASYNCMSG_SET 2


typedef void* asyncmsg_t;


int asyncmsg_initialize(asyncmsg_t *am, const char *addr, const int port, const int type);

int asyncmsg_send(const asyncmsg_t am, const void *data, const int size, const int flag);

int asyncmsg_recv(const asyncmsg_t am, void *data, const int size, const int flag);

int asyncmsg_finalize(const asyncmsg_t am);


#ifdef __cplusplus
}
#endif

#endif	//__ASYNCMSG_H__
