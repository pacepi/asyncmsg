#ifndef __AM_LOG_H__
#define __AM_LOG_H__


/*
 * am_log.h : log redirection and log level
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


#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif


#define am_log_with_tag(TAG,fmt,...)\
	do {\
		struct timeval tv;\
		gettimeofday(&tv, NULL);\
		printf("[%ld:%06ld] ", tv.tv_sec, tv.tv_usec);\
		printf("[%5s] ", TAG);\
		printf("[%s] [%s] [%d] ", __FILE__, __FUNCTION__, __LINE__);\
		printf(fmt, ##__VA_ARGS__);\
	} while(0);


#define am_log_short(fmt,...)\
	do {\
		struct timeval tv;\
		gettimeofday(&tv, NULL);\
		printf("[%ld:%06ld] ", tv.tv_sec, tv.tv_usec);\
		printf(fmt, ##__VA_ARGS__);\
	} while(0);


#define am_log_info(fmt,args...) am_log_with_tag("INFO",fmt,##args)


#define am_log_warn(fmt,args...) am_log_with_tag("WARN",fmt,##args)


#define am_log_trace() am_log_with_tag("TRACE","trace\n")


#define am_log_error(fmt,args...)\
	do {\
		am_log_with_tag("ERROR",fmt,##args);\
		fflush(stdout);\
		perror("[reason]");\
	} while(0)


#define am_log_fatal(fmt,args...)\
	do {\
		am_log_with_tag("FATAL",fmt,##args);\
		fflush(stdout);\
		abort();\
	} while(0)


#define am_log_flush() fflush(stdout)


int am_log_init(char *redirection_path);


#ifdef __cplusplus
}
#endif

#endif	//__AM_LOG_H__
