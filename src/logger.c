/* 
 * This file is part of Luna
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*******************************************************************************
 *
 *  Logging functionality (logger.c)
 *  ---
 *  Manage file and console logging
 *
 *  Created: 25.02.2011 11:25:36
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "luna.h"
#include "logger.h"

extern int errno;

int logger_level_to_string(luna_loglevel, char *, size_t);
int logger_format_timestamp(time_t, char *, size_t);

int
logger_init(luna_log **log, const char *file)
{
    FILE *temp_file = NULL;
    
    if (!(temp_file = fopen(file, "a")))
        return errno;

    if ((*log = malloc(sizeof(**log))) != NULL)
    {
        memset(*log, 0, sizeof(**log));
        strncpy((*log)->filename, file, sizeof((*log)->filename) - 1);
        (*log)->file = temp_file;

        return 0;        
    }

    return -1;
}


int
logger_destroy(luna_log *log)
{
    fclose(log->file);
    free(log);

    return 0;
}


int
logger_log(luna_log *log, luna_loglevel loglevel, const char *fmt, ...)
{
    va_list args;
    int retval = -1;
    int first = 0;
    int remaining = 0;
    char buffer[1024];
    char timestamp[128];
    char level[16];

    memset(buffer, 0, sizeof(buffer));
    memset(timestamp, 0, sizeof(timestamp));
    memset(level, 0, sizeof(level));

    /* Format timestamp and level to human readable strings */
    logger_level_to_string(loglevel, level, sizeof(level));
    logger_format_timestamp(time(NULL), timestamp, sizeof(timestamp));

    /* Print timestamp and level to buffer */
    first = snprintf(buffer, sizeof(buffer), LOG_FORMAT, timestamp, level);
    remaining = sizeof(buffer) - first;

    /* Write the actual message */
    va_start(args, fmt);
    retval = vsnprintf(buffer + first, remaining, fmt, args);
    va_end(args);

    /* Write out to console and file */
    printf("%s\n", buffer);
    fprintf(log->file, "%s\n", buffer);

    return retval + first;
}


int
logger_level_to_string(luna_loglevel loglevel, char *dest, size_t len)
{
    const char *string = NULL;

    switch (loglevel)
    {
        case LOGLEV_INFO:
            string = "INFO";
            break;

        case LOGLEV_WARNING:
            string = "WARNING";
            break;

        case LOGLEV_ERROR:
            string = "ERROR";
            break;

        default:
            string = "UNKNOWN";
            break;
    }

    strncpy(dest, string, sizeof(dest) - 1);

    return 0;
}


int
logger_format_timestamp(time_t stamp, char *dest, size_t len)
{
    char *format = TIMESTAMP_FORMAT;

#ifdef TIMESTAMP_LOCAL
    struct tm *tm = localtime(&stamp);
#else
    struct tm *tm = gmtime(&stamp);
#endif
    
    return strftime(dest, len, format, tm);
}
