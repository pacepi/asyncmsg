/*
 * am_input.c : process input address and build input list
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


#include "am.h"
#include "am_log.h"
#include "iniparser/iniparser.h"
#include "utils/list.h"


int am_input_load_conf(const char *conf)
{
    dictionary *ini = NULL;
    int nsec = 0;
    int i = 0;
    char *sec = NULL;
    char key[128] = {0};
    char *address = NULL;

    am_log_info("conf : %s\n", conf);
    ini = iniparser_load(conf);
    if (ini == NULL)
        am_log_warn("iniparser_load %s fail\n", conf);

    nsec = iniparser_getnsec(ini);
    if (nsec == 0) {
        am_log_warn("conf %s has no section\n", conf);
        iniparser_freedict(ini);
        return 0;
    }

    for (i = 0; i < nsec; i++) {
        sec = iniparser_getsecname(ini, i);
        if (sec == NULL)
            am_log_fatal("section %d invalid\n", i);

        memset(key, 0x00, sizeof(key));
        snprintf(key, sizeof(key), "%s:address", sec);
        address = iniparser_getstring(ini, key, NULL);
        if (address == NULL)
            am_log_fatal("key %s has no address\n", key);

        asyncmsg_t *am = am_find(sec, address);
        if (am == NULL) {
            am_log_info("am_add_new : %s : %s\n", sec, address);
            am_add_new(sec, address);
        } else {
            am_log_info("am %s : %s has alread inserted\n", sec, address);
        }
    }

    if (list_empty(am_get_list()))
        am_log_warn("am_list empty\n");

    iniparser_freedict(ini);

    return 0;
}
