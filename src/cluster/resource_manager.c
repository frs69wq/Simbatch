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

static double get_next_wakeup_time(cluster_t cluster, job_t * job)
{
    double waiting_time = DBL_MAX;
    // int ppid = MSG_process_self_PPID();

    *job = next_job_to_schedule(cluster);
    waiting_time =
	(*job != NULL)? (*job)->start_time - MSG_get_clock(): DBL_MAX;

    assert(waiting_time >= 0.0);
#ifdef DEBUG
    fprintf(stderr, "[%lf] Waiting time 2: %lf\n", 
	    MSG_get_clock(), waiting_time);
#endif
    
    return waiting_time;
}  


int SB_resource_manager(int argc, char ** argv)
{
    cluster_t cluster = (cluster_t) MSG_host_get_data(MSG_host_self());
    unsigned int cpt_supervisors = 0;
    double waiting_time = DBL_MAX;
    xbt_dynar_t pool_of_supervisors = xbt_dynar_new(sizeof(m_process_t), 
						    NULL);
    job_t job;

#ifdef LOG
    FILE * flog = config_get_log_file(HOST_NAME());
#endif
    
#ifdef VERBOSE
    fprintf(stderr, "Resource manager: ready\n");
#endif
    
    while (1)
    {
	m_task_t task = NULL;
	MSG_error_t ok = MSG_OK;
	
	/* 0.0 makes MSG_task_get_with_time_out blocking */
	if (waiting_time > 0.0)
	{
	    ok = MSG_task_get_with_time_out(&task, RSC_MNG_PORT, waiting_time);
	    if (ok != MSG_OK) 
	    {
		break;
	    }
	}
	
	/* No tasks in DBL_MAX of time, that's too long, bye */
	if ((task == NULL) && (waiting_time == DBL_MAX))
	    break;
	
	
	/*** default ***/
	/* Wake up because there is a job to send */
	if ((task == NULL) && (waiting_time < DBL_MAX))
	{
	    m_process_t supervisor;
	    unsigned int  * port = NULL;		

#ifdef DEBUG
	    fprintf(stderr, "[%lf]\t%20s\tIt's time, nb sup : %lu\n",
		    MSG_get_clock(), PROCESS_NAME(), 
		    xbt_dynar_length(pool_of_supervisors));
#endif
	
	    if (task) {MSG_task_destroy(task), task = NULL;}
	 
	    /* Handle the pool of supervisors */
	    if (xbt_dynar_length(pool_of_supervisors) == 0)
	    {
		char name[20];
		
		assert(cpt_supervisors < cluster->nb_nodes);
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
	    MSG_task_put(MSG_task_create("RUN", 0.0, 0.0, job),
			 MSG_host_self(), *port);
	    /* Update myself */
	    job->state = PROCESSING;
	    waiting_time = get_next_wakeup_time(cluster, &job);
	    continue;
	}


	/* Wake up, Something happened so I have to update myself */ 
	if (strcmp(task->name, "UPDATE") == 0)
	{
#ifdef DEBUG
	    fprintf(stderr, "[%lf]\t%20s\tUpdate\n", MSG_get_clock(), 
		    PROCESS_NAME());
#endif
	    MSG_task_destroy(task), task = NULL;
	    waiting_time = get_next_wakeup_time(cluster, &job);
	    continue;
	}
	
	/* A Supervisor has finished and is available again */
	if (!strcmp(task->name, "ATTACH"))
	{
	    m_process_t supervisor = MSG_task_get_data(task);
	    MSG_task_destroy(task), task = NULL;
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
	    waiting_time = get_next_wakeup_time(cluster, &job);
	    continue;
	}
    }
    
#ifdef VERBOSE
    xbt_dynar_free(&pool_of_supervisors);
    fprintf(stderr, "Pool of supervisors cleaned... ok\n");
#endif
    return EXIT_SUCCESS;
}


static int supervise(int argc, char ** argv)
{
#ifdef LOG
    FILE * flog = config_get_log_file(HOST_NAME());
#endif
    cluster_t cluster = (cluster_t) MSG_host_get_data(MSG_host_self());
    unsigned int * port = (unsigned int *) 
	MSG_process_get_data(MSG_process_self());

#ifdef LOG
    fprintf(flog, "[%lf]\t%20s\tHELLO!\n", MSG_get_clock(), PROCESS_NAME());
#endif
    while (1)
    {
	MSG_error_t err;
	job_t job = NULL;
	m_host_t * hosts = NULL;
	m_task_t task = NULL, pTask = NULL;
	double * comm = NULL, * comp = NULL;
	int i = 0;

	/* This process will end at DBL_MAX */
	err = MSG_task_get_with_time_out(&task, *port, DBL_MAX);
	/* Something strange occured */
	if (err != MSG_OK) 
	{
	    break;
	}
	if (task == NULL)
	    break;
        
	job = MSG_task_get_data(task);
	MSG_task_destroy(task), task = NULL;
	
	/* Send input data TODO: use put_with_alarm */
	if (job->input_size > 0.0)
	{
	    MSG_task_put(
		MSG_task_create("DATA_IN", 0.0, job->input_size * 1000000, 
				NULL),
		cluster->nodes[job->mapping[0]], NODE_PORT);
	} 
	/* PTask creation & execution */
	comm = calloc(job->nb_procs, sizeof(double));
	comp = malloc(job->nb_procs * sizeof(double));
	hosts = malloc(job->nb_procs * sizeof(m_host_t));
	for (i=0; i<job->nb_procs; i++)
	{
	    hosts[i] = cluster->nodes[job->mapping[i]];
	    comp[i] = MSG_get_host_speed(hosts[i]) * job->run_time;
	}
	pTask = MSG_parallel_task_create("job->name", job->nb_procs, hosts, 
					 comp, comm, NULL);
        
#ifdef LOG
	fprintf(flog, "[%lf]\t%20s\tProcessing job %s\n", MSG_get_clock(),
		PROCESS_NAME(), job->name);
#endif
	MSG_parallel_task_execute(pTask);
	
	xbt_free(hosts), hosts = NULL;
	xbt_free(comm), comm = NULL;
	xbt_free(comp), comp = NULL;
	MSG_task_destroy(pTask), pTask = NULL;
	
	/* Receive output data TODO: use get_with_alarm */
	if (job->output_size > 0.0)
	{
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
	MSG_task_put(MSG_task_create("ATTACH", 0.0, 0.0, MSG_process_self()),
		     MSG_host_self(), RSC_MNG_PORT);
	MSG_task_put(MSG_task_create("ACK", 0.0, 0.0, job),
		     MSG_host_self(), CLIENT_PORT);
    }

    xbt_free(port);
    return EXIT_SUCCESS;
}
