/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <xbt/dict.h>
#include <msg/msg.h>

#include "simbatch_config.h"
#include "utils.h"
#include "ports.h"
#include "job.h"
#include "node.h"


int SB_node(int argc, char ** argv) { 
#ifdef VERBOSE
    fprintf(stderr, "%s... ready\n", MSG_host_get_name(MSG_host_self())); 
#endif
    while (1) {
	// MSG_error_t err;
	m_task_t data = NULL;

#ifdef LOG
	FILE * flog;
#endif
	
	/* Waiting for input comm */
	MSG_task_get_with_time_out(&data, NODE_PORT, DBL_MAX);
	if (data) {
#ifdef LOG
	    flog = config_get_log_file(MSG_host_get_name(
					   MSG_task_get_source(data)));
	    if (!strcmp(data->name, "DATA_IN"))
		fprintf(flog, "[%lf]\t%20s\tData Received from %s\n", 
			MSG_get_clock(), PROCESS_NAME(), 
			MSG_process_get_name(MSG_task_get_sender(data)));
#endif
	    MSG_task_destroy(data), data = NULL;
	}
	else
	    break;

    }
    
    return EXIT_SUCCESS;
}
