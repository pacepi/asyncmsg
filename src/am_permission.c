/*
 * am_permission.c : generate a script for permission controlling
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
#include "am.h"
#include "am_log.h"
#include "iniparser/iniparser.h"
#include "utils/list.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define SHELLHEAD "#!/bin/sh\n"


int __am_permission_generate(int fd, int port, char *allowhost, char *prefix)
{
    char *host = NULL;
    char *hosts = NULL;
    char command[256] = {0};
    char chain[64] = {0};
    int ret = 0;

    am_log_info("port = %d, allowhost = %s\n", port,allowhost);
    memset(chain, 0x00, sizeof(chain));
    snprintf(chain, sizeof(chain), "%s_%d", prefix, port);

    /* write some info */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "#start configurarion %s\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    /* do Not jump to the old chain, may fail */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -D INPUT -t filter -p tcp --dport %d -j %s\n", port, chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }


    /* flush the old chain, may fail */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -F %s\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    /* remove the old chain, may fail */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -X %s\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    /* add the chain*/
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -N %s\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    /* jump to the new chain */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -A INPUT -t filter -p tcp --dport %d -j %s\n", port, chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    for (hosts = allowhost; ; hosts = NULL) {
        host = strtok(hosts, ",");
        if (host == NULL)
            break;
        /* add allowhost rules to new chain */
        memset(command, 0x00, sizeof(command));
        snprintf(command, sizeof(command), "iptables -A %s -s %s -j ACCEPT\n", chain, host);
        ret = write(fd, command, strlen(command));
        if (ret < 0) {
            am_log_error("write %s fail\n", command);
            return -errno;
        }
    }

    /* other host DROP */
    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "iptables -A %s -j DROP\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    memset(command, 0x00, sizeof(command));
    snprintf(command, sizeof(command), "#end configurarion %s\n\n", chain);
    ret = write(fd, command, strlen(command));
    if (ret < 0) {
        am_log_error("write %s fail\n", command);
        return -errno;
    }

    /* do Not jump to the old chain, may fail */
    return 0;
}


int __am_permission(const char *conf, char *prefix)
{
    dictionary *ini = NULL;
    int nsec = 0;
    int i = 0;
    int port = 0;
    int fd = -1;
    int ret = 0;
    char *sec = NULL;
    char key[128] = {0};
    char *allowhost = NULL;
    char *address = NULL;

    ini = iniparser_load(conf);
    if (ini == NULL)
        am_log_warn("iniparser_load %s fail\n", conf);

    nsec = iniparser_getnsec(ini);
    if (nsec == 0) {
        am_log_warn("conf %s has no section\n", conf);
        iniparser_freedict(ini);
        return 0;
    }

    /* unlink the orginal file if exits, it does NOT matter weather unlink fail or not */
    memset(key, 0x00, sizeof(key));
    snprintf(key, sizeof(key), "%s_permission.sh", prefix);
    unlink(key);
    fd = open(key, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        am_log_error("create %s fail\n", key);
        return -errno;
    }

    /*
    ret = ftruncate(fd, 0);
    if (ret < 0) {
    	am_log_error("ftruncate %s fail\n", key);
    	return -errno;
    }
    */

    ret = write(fd, SHELLHEAD, sizeof(SHELLHEAD) - 1);
    if (ret < 0) {
        am_log_error("write %s to %s fail\n", SHELLHEAD, key);
        return -errno;
    }

    for (i = 0; i < nsec; i++) {
        sec = iniparser_getsecname(ini, i);
        if (sec == NULL)
            am_log_fatal("section %d invalid\n", i);

        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:allowhost", sec);
        allowhost = iniparser_getstring(ini, key, NULL);
        if (allowhost == NULL) {
            am_log_info("key %s has no allowhost\n", key);
            continue;
        }

        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:address", sec);
        address = iniparser_getstring(ini, key, NULL);
        if (address == NULL)
            am_log_fatal("key %s has no address\n", key);
        port = atoi(strrchr(address, ':') + 1);
        if (port == 0) {
            am_log_info("key %s has no port\n", address);
            continue;
        }

        __am_permission_generate(fd, port, allowhost, prefix);
    }

    syncfs(fd);
    close(fd);
    iniparser_freedict(ini);

    return 0;
}


int am_input_permission(const char *conf)
{
    __am_permission(conf, "am_input");

    return 0;
}


int am_output_permission(const char *conf)
{
    __am_permission(conf, "am_output");

    return 0;
}
