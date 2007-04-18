#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>
#include <msg/msg.h>

#include "simbatch_config.h"
#include "utils.h"
#include "ports.h"
#include "job.h"

#include "plugin.h"
#include "plugin_input.h"

#include "external_load.h"

extern int DIET_MODE;

const char * SB_request_external_load(void) {
    char request[128];
    const char * filename;
    
    /*** External Load ***/
#ifdef VERBOSE
    fprintf(stderr, "Get external workload : ");
#endif
    
    sprintf(request, "/config/batch[@host=\"%s\"]/load/text()",\
	    MSG_host_get_name(MSG_host_self())); 
    filename = config_get_value(request);
    
#ifdef VERBOSE
    if (filename == NULL)
	fprintf(stderr, "no external workload\n");
    else
	fprintf(stderr, "%s\n", filename);
#endif
    
    return filename;
}

const char * SB_request_parser(void) {
    char request[128];
    const char * parserName;
    
    /*** External Load ***/
#ifdef VERBOSE
    fprintf(stderr, "Get workload parser: ");
#endif
    
    sprintf(request, "/config/batch[@host=\"%s\"]/parser/text()",\
	    MSG_host_get_name(MSG_host_self())); 
    parserName = config_get_value(request);

#ifdef VERBOSE
    if (parserName == NULL)
	fprintf(stderr, "no parser defined!\n");
    else
	fprintf(stderr, "%s\n", parserName);
#endif
    
    return parserName;
}


int SB_external_load(int argc, char ** argv) {
    const char ** param = (const char **)MSG_process_get_data(MSG_process_self());
    const char * filename = param[0];
    const char * parserName = param[1];
    void * handle = NULL;
    plugin_input plugin;
    xbt_fifo_t bag_of_tasks = NULL;
#ifdef LOG
    FILE * flog = config_get_log_file(HOST_NAME());
#endif
    
    xbt_free(param);
    launch_plugin(&handle, &plugin, parserName, "init_input");
    bag_of_tasks = plugin.create_list(filename, "task");
    close_plugin(handle);
    
    /* Now just send the job at time to the scheduler */
    if (bag_of_tasks == NULL)
	return EXIT_FAILURE;
    
    if (xbt_fifo_size(bag_of_tasks)) {
	double time = 0;
	job_t job = NULL;
	
#ifdef VERBOSE
	fprintf(stderr, "%s : workload ready - %d jobs\n", HOST_NAME(),
            xbt_fifo_size(bag_of_tasks));
#endif
	
	/* And the others */
	while ((job=(job_t)xbt_fifo_shift(bag_of_tasks))) {   
	    MSG_process_sleep(job->submit_time - time);
#ifdef LOG	
	    fprintf(flog, "[%lf]\t%20s\tSend %s to \"%s\"\n", 
		    MSG_get_clock(), PROCESS_NAME(),\
		    job->name, HOST_NAME());
#endif		
	    
	    MSG_task_put(MSG_task_create("SB_TASK", 0, 0, job), 
			 MSG_host_self(), CLIENT_PORT);
	    time = job->submit_time;
	}
	xbt_fifo_free(bag_of_tasks);
    }

    /* When everything has been submitted - ask question for DIET */ 
    if (DIET_MODE) {
	MSG_task_put(MSG_task_create("DIET_REQUEST", 0, 0, NULL), 
		     MSG_host_self(), CLIENT_PORT);
    }
    
    return EXIT_SUCCESS;  
}


