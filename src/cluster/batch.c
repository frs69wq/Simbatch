/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* Simgrid headers */
#include <xbt/asserts.h>
#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <xbt/dict.h>
#include <msg/msg.h>

/* Simbatch headers */
#include "simbatch_config.h"
#include "utils.h"
#include "ports.h"
#include "cluster.h"
#include "job.h"

/* Process definitions */
#include "external_load.h"
#include "resource_manager.h"
#include "batch.h"

/* Plugin definition */
#include "plugin.h"
#include "plugin_scheduler.h"


/********** Here it starts *************/
int SB_batch(int argc, char ** argv)
{
    const char * wld_filename;
    int jobCounter = 0;
    m_process_t resource_manager = NULL;
   
#ifdef OUTPUT
    char * out_file;
    FILE * fout;
#endif  
    
    
#ifdef LOG
    /*************** Create log file ****************/
    FILE * flog = config_get_log_file(HOST_NAME());
#endif
    
    cluster_t cluster = NULL;
    pluginInfo_t plugin;
    m_process_t wld_process;
    
    
    /**************** Configuration ******************/  
    
    /*** Plug in ***/
    plugin = SB_request_plugin("init", sizeof(plugin_scheduler));
    if (plugin == NULL)
    {
        simbatch_clean();
        xbt_die("Failed to load a plugin");
    }
#ifdef VERBOSE
    fprintf(stderr, "plugin name : %s\n", plugin->name);
#endif
    
    
    /*** Cluster ***/
#ifdef VERBOSE
    fprintf(stderr, "*** %s init *** \n", HOST_NAME());
#endif
    cluster = SB_request_cluster(argc, argv);
    if (cluster == NULL)
    {
        simbatch_clean();
        xbt_die("Failed to create the cluster");
    }
    MSG_host_set_data(MSG_host_self(), cluster);

    
    /*** External Load ***/
    wld_filename = SB_request_external_load();
    
    
    /* Test if config is still needed */
    config_close();
    
    
    /*** Output file ***/
#ifdef OUTPUT
    out_file = calloc(strlen(HOST_NAME()) + 5, sizeof(char));
    sprintf(out_file, "%s.out", HOST_NAME());
    
#ifdef VERBOSE
    fprintf(stderr,"Open outptut file : ");
#endif 

    fout = fopen(out_file, "w");
  
#ifdef VERBOSE
    if (fout == NULL)
        fprintf(stderr,"failed\n");
    fprintf(stderr,"%s\n", out_file);
#endif
    
#endif
    

    /****************** Create load ******************/
    if (wld_filename != NULL)
    {
#ifdef VERBOSE
	fprintf(stderr, "Create load... \n");
#endif
	wld_process = MSG_process_create("external_load", SB_external_load,
					 (void *)wld_filename, 
					 MSG_host_self());
    }
#ifdef VERBOSE
    else
	fprintf(stderr, "No load, dedicated platform\n");
#endif
    

    /************* Create resource manager ************/
    resource_manager = MSG_process_create("Resource manager", 
					  SB_resource_manager,
					  NULL, 
					  MSG_host_self());	


    /************* start schedule ***************/  
#ifdef LOG
    fprintf(flog, "[%lf]\t%20s\tWait for tasks\n", 
	    MSG_get_clock(), PROCESS_NAME());
#endif
    
 
    /* Receiving task from the client */
    while (1) 
    {
	m_task_t task = NULL;
	MSG_error_t err = MSG_OK;

	err = MSG_task_get_with_time_out(&task, CLIENT_PORT, DBL_MAX);
	 
	if (err != MSG_OK) // Error
	{
	    break;
	}

	if (task == NULL) // Normal exit (DBL_MAX timeout)
	    break;
	
	    
	/************ Schedule ************/
	if (!strcmp(task->name, "SB_TASK")) 
	{
	    job_t job = NULL;
	    
	    job = MSG_task_get_data(task);
#ifdef LOG	
	    fprintf(flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\"\n",
		    MSG_get_clock(), PROCESS_NAME(), job->name,
		    MSG_host_get_name(MSG_task_get_source(task)));
#endif
	    MSG_task_destroy(task), task = NULL;
	    job->id = jobCounter++;
	    
	    /* We manage too big jobs here to avoid useless memory
	     * operation and to simplify the plugin writing 
	     */
	    if (job->nb_procs > cluster->nb_nodes)
	    {
#ifdef LOG	
		fprintf(flog,
			"[%lf]\t%20s\t%s canceled: not enough ressource\n",
			MSG_get_clock(), PROCESS_NAME(), job->name);
#endif		
		continue;
	    }
		
	    /* We keep a trace of the task */
	    if (job->priority >= cluster->priority)
		job->priority = cluster->priority - 1;
	    
	    job->entry_time = MSG_get_clock();
	    job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
	    xbt_dynar_push(cluster->queues[job->priority], &job); 
	    
	    /* Then we ask to the plugin to schedule this new task */
//	    MSG_process_suspend(resource_manager);
	    ((plugin_scheduler_t) plugin->content)->schedule(cluster, job);
//	    MSG_process_resume(resource_manager);
	    
	    /* Notify the resource manager */
#ifdef DEBUG
	    fprintf(stderr, "[%lf]\t%20s\tSend an update to the RM\n", 
		    MSG_get_clock(), PROCESS_NAME());
#endif 
	    MSG_task_put(MSG_task_create("UPDATE", 0.0, 0.0, NULL),
			 MSG_host_self(), RSC_MNG_PORT);
	    
#ifdef GANTT
	    cluster_print(cluster);
#endif
	    continue;
	}
	

	/*** A task has been done ***/
	if (!strcmp(task->name, "ACK"))
	{
	    job_t job = NULL;
	    int it;

	    job = MSG_task_get_data(task);

#ifdef LOG	
	    fprintf(flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" \n",
		    MSG_get_clock(), PROCESS_NAME(), task->name,
		    MSG_process_get_name(MSG_task_get_sender(task)));
#endif
	    MSG_task_destroy(task), task = NULL;

#ifdef LOG
	    fprintf(flog, "[%lf]\t%20s\t%s done\n", 
		    MSG_get_clock(), PROCESS_NAME(), job->name);
#endif

	    //MSG_process_suspend(resource_manager);
	    job->state = DONE;
	    cluster_delete_done_job(cluster, job);
	    it = cluster_search_job(cluster, job->id, job->priority);
	    xbt_dynar_remove_at (cluster->queues[job->priority], it, NULL);

#ifdef OUTPUT
	    fprintf(fout, "%-15s\t%lf\t%lf\t%lf\t%lf\t\n",
		    job->name, job->entry_time, job->start_time, 
		    MSG_get_clock(), MSG_get_clock() - job->start_time);
#endif

	    xbt_free(job->mapping);
	    xbt_free(job);
	    MSG_task_put(MSG_task_create("UPDATE", 0.0, 0.0, NULL),
			 MSG_host_self(), RSC_MNG_PORT);
	    /* The system becomes stable again, so we can now reschedule */
	    ((plugin_scheduler_t) 
	     plugin->content)->reschedule(cluster, ((plugin_scheduler_t) 
						    plugin->content)->schedule );
	    //MSG_process_resume(resource_manager);
	    continue;
	}
    }
    
    // MSG_process_kill(resource_manager);

    /* Clean */  

#ifdef OUTPUT
#ifdef VERBOSE
    fclose(fout);
    fprintf(stderr, "Close %s... ok\n", out_file);
    free(out_file);
#endif
#endif

    cluster_destroy(&cluster);
#ifdef VERBOSE
    fprintf(stderr, "%s cleaned\n", HOST_NAME());
#endif

    
    return EXIT_SUCCESS;
}
