/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "simbatch_config.h"
#include "scheduling.h"
#include "utils.h"
#include "ports.h"
#include "cluster.h"
#include "job.h"

#include "resource_manager.h"

static int supervise(int argc, char ** argv);

static double get_next_wakeup_time(cluster_t cluster, job_t * job) {
    double waiting_time = DBL_MAX;
    // int ppid = MSG_process_self_PPID();

    *job = next_job_to_schedule(cluster);
    waiting_time =
	(*job != NULL)? (*job)->start_time - MSG_get_clock(): DBL_MAX;

    assert(waiting_time >= 0.0);

/*     if (waiting_time != DBL_MAX) */
/* 	fprintf(stderr, "[%lf] Waiting time: %lf\n", MSG_get_clock(), waiting_time); */
/*     else */
/* 	fprintf(stderr, "[%lf] Waiting time: %s\n", MSG_get_clock(), "DBL_MAX"); */

    return waiting_time;
}  


int SB_resource_manager(int argc, char ** argv) {
    cluster_t cluster = (cluster_t) MSG_host_get_data(MSG_host_self());
    unsigned int cpt_supervisors = 0;
    double waiting_time = DBL_MAX;
    xbt_dynar_t pool_of_supervisors = xbt_dynar_new(sizeof(m_process_t), 
						    NULL);
    xbt_fifo_t msg_stack = xbt_fifo_new();
    job_t job;
    m_task_t task = NULL;
    MSG_error_t ok = MSG_OK;

    
#ifdef LOG
    FILE * flog = config_get_log_file(HOST_NAME());
#endif
    
#ifdef VERBOSE
    fprintf(stderr, "Resource manager: ready\n");
#endif
    
    while (1) {
	/* 0.0 makes MSG_task_get_with_time_out blocking */
	ok = (waiting_time == 0.0)? MSG_TRANSFER_FAILURE:
	    MSG_task_get_with_time_out(&task, RSC_MNG_PORT, waiting_time);
        
	if (ok == MSG_TRANSFER_FAILURE) {
            
	    /* No tasks in DBL_MAX of time, that's too long, bye */	    
	    if (waiting_time == DBL_MAX) { xbt_fifo_free(msg_stack); break; }
	    
	    /* Wake up because there is a job to send */
	    if (waiting_time < DBL_MAX) {
		m_process_t supervisor;
		unsigned int  * port = NULL;		
                
#ifdef DEBUG
		fprintf(stderr, "[%lf]\t%20s\tIt's time, nb sup : %lu\n",
			MSG_get_clock(), PROCESS_NAME(), 
			xbt_dynar_length(pool_of_supervisors));
#endif
		
		/* Handle the pool of supervisors */
		if (xbt_dynar_length(pool_of_supervisors) == 0) {
		    char name[20];
		    
		    //assert(cpt_supervisors < cluster->nb_nodes);
		    port = malloc(sizeof(unsigned int));
		    *port = SUPERVISOR_PORT + cpt_supervisors;
		    sprintf(name, "Supervisor%u", cpt_supervisors);
		    supervisor = MSG_process_create(name, supervise, port, 
						    MSG_host_self());
		    xbt_dynar_push(pool_of_supervisors, &supervisor);
		    ++cpt_supervisors;
		}
                
		xbt_dynar_shift(pool_of_supervisors, &supervisor);
#ifdef LOG
		fprintf(flog, "[%lf]\t%20s\tDetach %s\n", MSG_get_clock(),
			PROCESS_NAME(), MSG_process_get_name(supervisor));
#endif
#ifdef DEBUG
		fprintf(stderr, "[%lf]\t%20s\tnb supervisors : %lu\n",
			MSG_get_clock(), PROCESS_NAME(), 
			xbt_dynar_length(pool_of_supervisors));
#endif	    
		/* Send the job to the supervisor that will manage it */
		port = (unsigned int *)MSG_process_get_data(supervisor);
		job->state = PROCESSING;
		MSG_task_put(MSG_task_create("RUN", 0.0, 0.0, job),
			     MSG_host_self(), *port);
	    }
	}
	
	
	if (ok == MSG_OK) {
            /* using a stack for avoiding losses in case of 
               multiple incoming messages */
            xbt_fifo_push(msg_stack, task);
	    while (MSG_task_Iprobe(RSC_MNG_PORT)) {
                task = NULL;
                MSG_task_get(&task, RSC_MNG_PORT);
                xbt_fifo_push(msg_stack, task);
            }
                    
            xbt_fifo_sort(msg_stack);
            
            while (xbt_fifo_size(msg_stack)) {
                task = xbt_fifo_shift(msg_stack);
                
                if (!strcmp(task->name, "SB_UPDATE")) {
#ifdef DEBUG
                    fprintf(stderr, "[%lf]\t%20s\tUpdate\n", MSG_get_clock(), 
                            PROCESS_NAME());
#endif
                }
                
                /* A Supervisor has finished and is available again */
                else if (!strcmp(task->name, "RM_ATTACH")) {
                    m_process_t supervisor = MSG_task_get_data(task);
#ifdef LOG
                    fprintf(flog, "[%lf]\t%20s\tAttach %s\n", MSG_get_clock(),
                            PROCESS_NAME(), MSG_process_get_name(supervisor));
#endif
                    xbt_dynar_push(pool_of_supervisors, &supervisor);
#ifdef DEBUG
                    fprintf(stderr, "[%lf]\t%20s\tnb supervisors : %lu\n", 
                            MSG_get_clock(), PROCESS_NAME(), 
                            xbt_dynar_length(pool_of_supervisors));
#endif
                }
                MSG_task_destroy(task); 
                task = NULL;
            }
        }
        
	waiting_time = get_next_wakeup_time(cluster, &job);
    }
    
#ifdef VERBOSE
    xbt_dynar_free(&pool_of_supervisors);
    fprintf(stderr, "Pool of supervisors cleaned... ok\n");
#endif
    return EXIT_SUCCESS;
}


static int supervise(int argc, char ** argv) {
    cluster_t cluster = (cluster_t) MSG_host_get_data(MSG_host_self());
    unsigned int * port = (unsigned int *) 
	MSG_process_get_data(MSG_process_self());
    MSG_error_t err;
    m_task_t task = NULL;

#ifdef LOG
    FILE * flog = config_get_log_file(HOST_NAME());
#endif

    while (1) {

	err = MSG_task_get_with_time_out(&task, *port, DBL_MAX);
       
	if (err == MSG_TRANSFER_FAILURE) { break; }

        if (err == MSG_OK) {
	    job_t job = NULL;
	    m_host_t * hosts = NULL;
	    m_task_t pTask = NULL;
	    double * comm = NULL, * comp = NULL;
	    int i = 0;

	    job = MSG_task_get_data(task);
	    MSG_task_destroy(task); 
	    task = NULL;
           	
	    /* Send input data TODO: use put_with_alarm */
            if (job->input_size > 0.0) {
		MSG_task_put(
		    MSG_task_create("DATA_IN", 0.0, job->input_size * 1000000,
				    NULL),
		    cluster->nodes[job->mapping[0]], NODE_PORT);
            }

	    /* PTask creation & execution */
	    comm = calloc(job->nb_procs, sizeof(double));
	    comp = malloc(job->nb_procs * sizeof(double));
	    hosts = malloc(job->nb_procs * sizeof(m_host_t));
	    for (i=0; i<job->nb_procs; i++) {
		hosts[i] = cluster->nodes[job->mapping[i]];
		comp[i] = MSG_get_host_speed(hosts[i]) * job->run_time;
	    }
	    pTask = MSG_parallel_task_create(job->name, job->nb_procs, hosts, 
					     comp, comm, NULL);

#ifdef LOG
	    fprintf(flog, "[%lf]\t%20s\tProcessing job %s end: %lf\n", 
                    MSG_get_clock(), PROCESS_NAME(), job->name, 
                    MSG_get_clock() + job->run_time);
#endif

	    // MSG_parallel_task_execute(pTask);
	    MSG_process_sleep(job->run_time);

	    xbt_free(hosts); hosts = NULL;
	    xbt_free(comm); comm = NULL;
	    xbt_free(comp); comp = NULL;
	    MSG_task_destroy(pTask); pTask = NULL;
	
	    /* Receive output data TODO: use get_with_alarm */
	    if (job->output_size > 0.0) {
                MSG_task_put(
		    MSG_task_create("DATA_OUT", 0.0, job->output_size * 1000000,
				    NULL),
		    cluster->nodes[job->mapping[0]], NODE_PORT);
		
#ifdef LOG
		fprintf(flog, "[%lf]\t%20s\tReceive output data from %s\n",
			MSG_get_clock(), PROCESS_NAME(),
			MSG_host_get_name(cluster->nodes[job->mapping[0]]));
#endif
	    }

	    /* Finish - ask to be in the pool again */
	    MSG_task_put(
                MSG_task_create("RM_ATTACH", 0.0, 0.0, MSG_process_self()),
                MSG_host_self(), RSC_MNG_PORT);
	    
            MSG_task_put(MSG_task_create("SB_ACK", 0.0, 0.0, job),
			 MSG_host_self(), CLIENT_PORT);
	}
    }

    xbt_free(port);
    return EXIT_SUCCESS;
}
