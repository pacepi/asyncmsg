/*
 * am_log.c : log redirection
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


#include "am_log.h"


int am_log_init(char *redirection_path)
{
    fflush(stdout);
    fflush(stderr);
    if (redirection_path == NULL) {
        if (freopen("/dev/null", "a", stdout) == NULL) {
            perror("freopen stdout");
            return -errno;
        }

        if (freopen("/dev/null", "a", stderr) == NULL) {
            perror("freopen stderr");
            return -errno;
        }
    }
    else {
        if (freopen(redirection_path, "a+", stdout) == NULL) {
            perror("freopen stdout");
            return -errno;
        }

        if (freopen(redirection_path, "a+", stderr) == NULL) {
            perror("freopen stderr");
            return -errno;
        }
    }

    if (freopen("/dev/null", "r", stdin) == NULL) {
        perror("freopen stdin");
        return -errno;
    }

    return 0;
}
