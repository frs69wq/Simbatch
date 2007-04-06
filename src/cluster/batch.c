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
#include "scheduling.h"

/* Process definitions */
#include "external_load.h"
#include "resource_manager.h"
#include "batch.h"

/* Plugin definition */
#include "plugin.h"
#include "plugin_scheduler.h"

#define UPDATE_RSC_MNG()  ({\
    MSG_task_put(MSG_task_create("UPDATE", 0.0, 0.0, NULL),\
		 MSG_host_self(), RSC_MNG_PORT);\
})

extern unsigned long int DIET_PARAM[4];
extern const char * DIET_FILE; 
extern int DIET_MODE;

double NOISE=0.0;

void xbt_fifo_sort(xbt_fifo_t fifo);

void xbt_fifo_sort(xbt_fifo_t fifo) {
    auto int stringCmp(const void * t1, const void * t2);
    int stringCmp(const void * t1, const void * t2) { 
        return strcmp((*((m_task_t *) t1))->name, (*((m_task_t *) t2))->name);
    }

    int i = 0;
    m_task_t * messages =  (m_task_t *)xbt_fifo_to_array(fifo);
    
    // fprintf(stderr, "qsort %d\n", xbt_fifo_size(fifo)); 
    qsort(messages, xbt_fifo_size(fifo), sizeof(m_task_t), stringCmp);
    for (i=0; i<xbt_fifo_size(fifo); ++i) {
        // fprintf(stderr, "%s\t", messages[i]->name);
        /* ok, it's a bit dirty */
        xbt_fifo_shift(fifo);
        xbt_fifo_push(fifo, messages[i]);
    }
    xbt_free(messages);
}

/********** Here it starts *************/
int SB_batch(int argc, char ** argv) {
    const char * wld_filename;
    int jobCounter = 0;
    m_task_t task = NULL;
    m_process_t resource_manager = NULL;
    MSG_error_t err = MSG_OK;   
    xbt_fifo_t msg_stack = NULL;

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
    if (plugin == NULL) {
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
    if (cluster == NULL) {
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
    if (wld_filename != NULL) {
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
    
    msg_stack = xbt_fifo_new();
    /* Receiving messages and put them in a stack */
    while (1) {
        err = MSG_task_get_with_time_out(&task, CLIENT_PORT, DBL_MAX);	
        if (err == MSG_TRANSFER_FAILURE) {
            if (!msg_stack) xbt_fifo_free(msg_stack);
            break;
        }
        if (err == MSG_OK) {
            xbt_fifo_push(msg_stack, task);
            while (MSG_task_Iprobe(CLIENT_PORT)) {
                task = NULL;
                MSG_task_get(&task, CLIENT_PORT);
                xbt_fifo_push(msg_stack, task);
            }
        }
        // fprintf(stderr, "%d\n", xbt_fifo_size(msg_stack));
        xbt_fifo_sort(msg_stack);
        // fprintf(stderr,"\n");

        /* retrieve messages from the stack */
        while (xbt_fifo_size(msg_stack)) {
            task = xbt_fifo_shift(msg_stack);
            // printf("\t%s\t%d\n", task->name, xbt_fifo_size(msg_stack)); 

            /************ Schedule ************/
            if (!strcmp(task->name, "SB_TASK")) {
		job_t job = NULL;
		
		job = MSG_task_get_data(task);
		job->id = jobCounter++;

#ifdef LOG	
		fprintf(flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\"\n",
			MSG_get_clock(), PROCESS_NAME(), job->name,
			MSG_host_get_name(MSG_task_get_source(task)));
#endif
			
		if (job->nb_procs <= cluster->nb_nodes) {
		    /* We keep a trace of the task */
		    if (job->priority >= cluster->priority)
			job->priority = cluster->priority - 1;
		    
		    job->entry_time = MSG_get_clock();
		    /* Noise */
		    job->run_time += NOISE;
		    job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
		    xbt_dynar_push(cluster->queues[job->priority], &job); 
		    
		    /* Then we ask to the plugin to schedule this new task */
		    ((plugin_scheduler_t) plugin->content)->schedule(cluster, job);
		}
#ifdef LOG	
		else {
		    fprintf(flog,
			    "[%lf]\t%20s\t%s canceled: not enough ressource\n",
			    MSG_get_clock(), PROCESS_NAME(), job->name);
		}
#endif		
#ifdef GANTT
                cluster_print(cluster);
#endif
            }	    
	    
	    /*** A task has been done ***/
            else if (!strcmp(task->name, "ACK")) {
                job_t job = NULL;
                int it;
                
                job = MSG_task_get_data(task);
		
#ifdef LOG	
                fprintf(flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" \n",
                        MSG_get_clock(), PROCESS_NAME(), task->name,
                        MSG_process_get_name(MSG_task_get_sender(task)));
                fprintf(flog, "[%lf]\t%20s\t%s done\n", 
                        MSG_get_clock(), PROCESS_NAME(), job->name);
#endif
                
                job->state = DONE;
                cluster_delete_done_job(cluster, job);
                it = cluster_search_job(cluster, job->id, job->priority);
                xbt_dynar_remove_at (cluster->queues[job->priority], it, NULL);
                
#ifdef OUTPUT
                fprintf(fout, "%-15s\t%lf\t%lf\t%lf\t%lf\t\n",
                        job->name, job->entry_time, 
                        job->start_time + NOISE, MSG_get_clock(), 
                        MSG_get_clock() - job->start_time - NOISE);
#endif
                
                xbt_free(job->mapping);
                xbt_free(job);
                
                /* The system becomes stable again, so we can now reschedule */
                ((plugin_scheduler_t) 
                 plugin->content)->reschedule(cluster, ((plugin_scheduler_t) 
                                                        plugin->content)->schedule );
            }
	    
            
	    /* Diet request - to do with */
            else if (!strcmp(task->name, "DIET_REQUEST")) {
                FILE * fdiet = fopen(DIET_FILE, "a"); 
                slot_t * slots = NULL;
                int i = 0;
                
                if (!fdiet) {
                    DIET_MODE = 0;
#ifdef VERBOSE
                    fprintf(stderr, "%s: %s\n", DIET_FILE, strerror(errno));
#endif
                }
                
                for (i=0; i<=2; i+=2) {
                    if (DIET_PARAM[i+1] > cluster->nb_nodes) 
                        DIET_PARAM[i+1] = cluster->nb_nodes;
                    slots = find_a_slot(cluster, DIET_PARAM[i+1],
                                        MSG_get_clock(), DIET_PARAM[i]);
                    fprintf(fdiet, "[%lf] DIET answer : %lf\n", 
                            MSG_get_clock(), slots[0]->start_time);
                    xbt_free(slots), slots = NULL;
                }
                
                fclose(fdiet);
            }
            MSG_task_destroy(task);
            task = NULL;
        }
        UPDATE_RSC_MNG();
    }
    
    
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
