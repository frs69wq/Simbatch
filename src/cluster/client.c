/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

/* This file is an example of what could be a client */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xbt/fifo.h>
#include <xbt/dict.h>
#include <msg/msg.h>

#include "simbatch_config.h"
#include "utils.h"
#include "plugin.h"
#include "plugin_input.h"
#include "ports.h"
#include "job.h"
#include "client.h"


int SB_client(int argc, char ** argv)
{
    m_host_t sched = NULL;
    xbt_fifo_t bag_of_tasks = NULL;
    void * handle = NULL;
    plugin_input plugin;
#ifdef LOG
    FILE * flog = NULL;
#endif

    if (argc!=5) 
    {
	fprintf(stderr, "Client has a bad definition\n");
	exit(1);
    }
    
    sched = MSG_get_host_by_name(argv[4]);
    if (!sched) 
    {
	fprintf(stderr, "Unknown host %s. Stopping Now!\n", argv[4]);
	exit(2);
    }
    
#ifdef LOG
    flog = config_get_log_file(sched->name);
#endif

    launch_plugin(&handle, &plugin, "../lib/input/libwld.so", "init_input");
    bag_of_tasks = plugin.create_list("./workload/seed/1.wld", "client");
    close_plugin(handle);
    
    /* Now just send the job at time to the scheduler */
    if (bag_of_tasks == NULL)
	return 1;
    
    if (xbt_fifo_size(bag_of_tasks))
    {
	double time = 0;
	job_t job = NULL;
	
#ifdef VERBOSE
	fprintf(stderr, "%s : workload ready\n", HOST_NAME());
#endif
	
	/* And the others */
	while ((job=(job_t)xbt_fifo_shift(bag_of_tasks)))
	{   
	    MSG_process_sleep(job->submit_time - time);
	    
#ifdef LOG	
	    fprintf(flog, "[%lf]\t%16s\t Send %s to \"%s\"\n",
		    MSG_get_clock(), PROCESS_NAME(), job->name, sched->name);
#endif		
	    
	    MSG_task_put(MSG_task_create("SB_TASK", 0.0, 0.0, job), 
			 sched, CLIENT_PORT);
	    time = job->submit_time;
	}
	xbt_fifo_free(bag_of_tasks);
    }
    
    return EXIT_SUCCESS;
}

