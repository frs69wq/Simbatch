#ifndef _UTILS_H_
#define _UTILS_H_

#include <msg/msg.h>

/* Return the tab's size */
#define TAB_SIZE(t) sizeof(t)/sizeof(*t)

/* Return the name of the host calling the macro */
#define HOST_NAME() MSG_host_get_name(MSG_host_self())

/* Return the name of the process calling the macro */
#define PROCESS_NAME() MSG_process_get_name(MSG_process_self())

#endif
