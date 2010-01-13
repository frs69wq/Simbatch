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
#include <xbt/ex.h>
#include <msg/msg.h>

#include "simbatch_config.h"
#include "scheduling.h"
#include "utils.h"
#include "cluster.h"
#include "job.h"

#include "resource_manager.h"

static int
supervise(int argc, char **argv);

static double
get_next_wakeup_time(m_cluster_t cluster, job_t *job)
{
  double waiting_time = DBL_MAX;

  *job = next_job_to_schedule(cluster);
  
  if(*job != NULL) {
    waiting_time = (*job)->start_time - MSG_get_clock();
  }

  //can happen because of double approx
  if (waiting_time < 0.0) {
#ifdef DEBUG
    printf("[%lf] waiting_time: %lf, job: %s startTime: %lf\n",
	   MSG_get_clock(), waiting_time, (*job)->name, (*job)->start_time);
#endif
    waiting_time = 0.0;
  }
  assert(waiting_time >= 0.0);

  return waiting_time;
}  


int
SB_resource_manager(int argc, char **argv)
{
  m_cluster_t cluster = (m_cluster_t) MSG_host_get_data(MSG_host_self());
  unsigned int cpt_supervisors = 0;
  double waiting_time = DBL_MAX;
  xbt_dynar_t pool_of_supervisors = xbt_dynar_new(sizeof(m_process_t), 
						  NULL);
  xbt_fifo_t msg_stack = xbt_fifo_new();
  job_t job = NULL;
  m_task_t task = NULL;
  char name[256];
  MSG_error_t err = MSG_OK;
  
#ifdef LOG
  FILE * flog = config_get_log_file(HOST_NAME());
#endif
    
#ifdef VERBOSE
  fprintf(stderr, "Resource manager: ready\n");
#endif
  
  sprintf(name, "resource_manager-%s", HOST_NAME());
  
  while (1) {
    //0.0 makes MSG_task_get_with_time_out blocking
    if (waiting_time == 0.0) {
      err = MSG_TIMEOUT_FAILURE;
    }
    else {
      err = MSG_task_receive_with_timeout(&task, name, waiting_time);
    }
    
    if (err == MSG_TIMEOUT_FAILURE) {
      //if we reach DBL_MAX, we should exit 
      if (waiting_time == DBL_MAX) { 
	xbt_fifo_free(msg_stack); 
	break; 
      }
      
      //otherwise, there is a task to execute
      m_process_t supervisor;
      unsigned int  *port = NULL;
      job->state = PROCESSING;
#ifdef DEBUG
      fprintf(stderr, "[%lf]\t%20s\tIt's time, nb sup : %lu\n",
	      MSG_get_clock(), PROCESS_NAME(), 
	      xbt_dynar_length(pool_of_supervisors));
#endif
		
      /* Handle the pool of supervisors */
      if (xbt_dynar_length(pool_of_supervisors) == 0) {
	char name[20];
	port = malloc(sizeof(unsigned int));
	*port = cpt_supervisors;
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
      char supervisor_MB[256];
      port = (unsigned int *)MSG_process_get_data(supervisor);
      sprintf(supervisor_MB, "supervisor-%s-%u", HOST_NAME(), *port);
      MSG_task_send(MSG_task_create("RUN", 0.0, 0.0, job),
		   supervisor_MB);
    }
    
    if (err == MSG_OK) {
      xbt_fifo_push(msg_stack, task);
      while (MSG_task_listen(name)) {
	task = NULL;
	MSG_task_receive(&task, name);
	xbt_fifo_push(msg_stack, task);
      }
      
      xbt_fifo_alphabetically_sort(msg_stack);
      
      while (xbt_fifo_size(msg_stack)) {
	task = xbt_fifo_shift(msg_stack);
	if (!strcmp(task->name, "SB_UPDATE")) {
#ifdef DEBUG
	  fprintf(stderr, "[%lf]\t%20s\tUpdate\n", MSG_get_clock(), 
		  PROCESS_NAME());
#endif
	} 
	else if (!strcmp(task->name, "RM_ATTACH")) {
	  /* A Supervisor has finished and is available again */
	  m_process_t supervisor = (m_process_t)MSG_task_get_data(task);
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


static int
supervise(int argc, char **argv)
{
  m_cluster_t cluster = (m_cluster_t)MSG_host_get_data(MSG_host_self());
  unsigned int *port = (unsigned int *)MSG_process_get_data(MSG_process_self());
  char name[256];
  char batch_MB[256];
  char resource_manager_MB[256];
  m_task_t task = NULL;
  job_t job = NULL;
  MSG_error_t err = MSG_OK;
  double sleep_duration;
  
#ifdef LOG
  FILE * flog = config_get_log_file(HOST_NAME());
#endif
  
  sprintf(name, "supervisor-%s-%u", HOST_NAME(), *port);
  sprintf(batch_MB, "batch-%s", HOST_NAME());
  sprintf(resource_manager_MB, "resource_manager-%s", HOST_NAME());
  while (1) {
    err = MSG_task_receive_with_timeout(&task, name, DBL_MAX);
    if (err == MSG_TIMEOUT_FAILURE) {
      break;
    }
    
    job = (job_t)MSG_task_get_data(task);
    MSG_task_destroy(task); 
    task = NULL;
    
    /* Send input data TODO: use put_with_alarm */
    char node_MB[256];
    sprintf(node_MB, "Node-%s", MSG_host_get_name(cluster->nodes[job->mapping[0]]));
    if (job->input_size > 0.0) {
      MSG_task_send(MSG_task_create("DATA_IN", 0.0,
				   job->input_size, NULL),
		   node_MB);
    }
    
    sleep_duration = job->run_time;

    /* take into account the fact that the walltime can be 
       greater than runtime and kill the job the earlier */
    if (job->wall_time < sleep_duration) {
      sleep_duration = job->wall_time;
    }

    //deadline will not be met
    if (job->deadline > MSG_get_clock() + sleep_duration) {
      sleep_duration = job->deadline - MSG_get_clock();
    }

#ifdef LOG
    fprintf(flog, "[%lf]\t%20s\tStart Processing job %s end: %lf\n", 
	    MSG_get_clock(), PROCESS_NAME(), job->name, 
	    MSG_get_clock() + sleep_duration);
#endif
    
    MSG_process_sleep(sleep_duration);
    
    /* Receive output data */
    if (job->output_size > 0.0) {
      MSG_task_send(MSG_task_create("DATA_OUT", 0.0,
				   job->output_size, NULL),
		   node_MB);
#ifdef LOG
      fprintf(flog, "[%lf]\t%20s\tReceive output data from %s\n",
	      MSG_get_clock(), PROCESS_NAME(),
	      MSG_host_get_name(cluster->nodes[job->mapping[0]]));
#endif
    }
    
    //Finish - ask to be in the pool again
    MSG_task_send(MSG_task_create("RM_ATTACH", 0.0, 0.0,
				 MSG_process_self()),
		 resource_manager_MB); 
    //Tell the batch the job is finished
    MSG_task_send(MSG_task_create("SB_ACK", 0.0, 0.0, job),
		 batch_MB);
  }
  xbt_free(port);
  return EXIT_SUCCESS;
}
