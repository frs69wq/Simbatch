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
    pluginInfo_t plugin = NULL;
    plugin_scheduler_t scheduler = NULL;
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
    scheduler = plugin->content;
    
    /*** Cluster ***/
#ifdef VERBOSE
    fprintf(stderr, "*** %s init *** \n", HOST_NAME());
#endif
    cluster = SB_request_cluster(argc, argv);
    if (cluster == NULL) {
        simbatch_clean();
        xbt_die("Failed to create the cluster");
    }
#ifdef VERBOSE
    fprintf(stderr, "Number of nodes: %d\n", cluster->nb_nodes);
#endif
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
#ifdef VERBOSE
    fprintf(stderr, "%s... ready\n", MSG_host_get_name(MSG_host_self())); 
#endif

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
            
            // fprintf(stderr, "%d\n", xbt_fifo_size(msg_stack));
            xbt_fifo_sort(msg_stack);
            
            /* retrieve messages from the stack */
            while (xbt_fifo_size(msg_stack)) {
                task = xbt_fifo_shift(msg_stack);
                // fprintf(stderr, "\t%s\n", task->name); 
                
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
                        
                        /* ask to the plugin to schedule and accept this new task */
                        scheduler->accept(cluster, job,
                                          scheduler->schedule(cluster, job));
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
                
                /*** Reservation handler ***/
                else if (!strcmp(task->name, "SB_RES")) {
                    job_t job = NULL;
                    slot_t * slot = NULL;
                    job = MSG_task_get_data(task);
                    job->id = jobCounter++;
                    int it = 0;
                    
#ifdef LOG	
                    fprintf(flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\"\n",
                            MSG_get_clock(), PROCESS_NAME(), job->name,
                            MSG_host_get_name(MSG_task_get_source(task)));
#endif
                    slot = find_a_slot(cluster, job->nb_procs,
                                       job->start_time, job->wall_time);
                    printf("Slot: \n");
                    for (it=0; it<cluster->nb_nodes; ++it) { 
                        printf("\tNode: %d\n", slot[it]->node);
                        printf("\tPosition: %d\n", slot[it]->position);
                        printf("\tStart: %lf\n", slot[it]->start_time);
                        printf("\tDuration: %lf\n", slot[it]->duration);
                    }
                    /* check reservation validity */
                    if (slot[job->nb_procs-1]->start_time == job->start_time) {
                        job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
                        xbt_dynar_push(cluster->reservations, &job);
                        /* insert reservation into the Gantt chart */
                        job->start_time = slot[0]->start_time;
                        for (it=0; it<job->nb_procs; ++it) {
                            job->mapping[it] = slot[it]->node; 
                            xbt_dynar_insert_at(cluster->waiting_queue[slot[it]->node], 
                                                slot[it]->position, &job);
                        }
                        cluster_print(cluster);
                    }
                    else
                        printf("Reservation impossible");
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
              
                    it = cluster_search_job(cluster, job->id, 
                                            cluster->reservations);
                    if (it == -1) { 
                        it = cluster_search_job(cluster, job->id, 
                                                cluster->queues[job->priority]);
                        xbt_dynar_remove_at(cluster->queues[job->priority], 
                                             it, NULL);
                    }
                    else { xbt_dynar_remove_at(cluster->reservations, it, NULL); }
#ifdef OUTPUT
                    fprintf(fout, "%-15s\t%lf\t%lf\t%lf\t%lf\t\n",
                            job->name, job->entry_time, 
                            job->start_time + NOISE, MSG_get_clock(), 
                            MSG_get_clock() - job->start_time - NOISE);
#endif
                    
                    xbt_free(job->mapping);
                    xbt_free(job);
                    
                    /* The system becomes stable again, we can now reschedule */
                    scheduler->reschedule(cluster, scheduler);
                }
                
                
                /* Diet request - to do with */
                else if (!strcmp(task->name, "DIET_REQUEST")) {
                    FILE * fdiet = fopen(DIET_FILE, "w"); 
                    slot_t * slots = NULL;
                    int i = 0;
                    
                    if (!fdiet) {
                        DIET_MODE = 0;
#ifdef VERBOSE
                        fprintf(stderr, "%s: %s\n", DIET_FILE, strerror(errno));
#endif
                    }
                    
                    for (i=0; i<=2; i+=2) {
                        job_t job =  xbt_malloc(sizeof(*job));
                      
                        strcpy(job->name, "DIET");
                        job->submit_time = MSG_get_clock();
                        job->input_size = 0.0; 
                        job->output_size = 0.0;
                        job->priority = 0.0;         
                        
                        if (DIET_PARAM[i+1] > cluster->nb_nodes) 
                            DIET_PARAM[i+1] = cluster->nb_nodes;
                       
                        job->nb_procs = DIET_PARAM[i+1];
                        job->wall_time = DIET_PARAM[i];
                        job->run_time = DIET_PARAM[i];
                        job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
                        // slots = find_a_slot(cluster, DIET_PARAM[i+1],
                        //                    MSG_get_clock(), DIET_PARAM[i]);
                        slots = scheduler->schedule(cluster, job);
                        fprintf(fdiet, "[%lf] DIET answer : %lf\n", 
                                MSG_get_clock(), slots[0]->start_time);
                        xbt_free(slots), slots = NULL;
                        xbt_free(job->mapping);
                        xbt_free(job);
                    }       
                    fclose(fdiet);
                }
                MSG_task_destroy(task);
                task = NULL;
            }
            UPDATE_RSC_MNG();
        }
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
