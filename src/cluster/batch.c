/****************************************************************************/
/* This file is part of the Simbatchproject                                 */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include <errno.h>

/* Simgrid headers */
#include <xbt/asserts.h>
#include <xbt/ex.h>
#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <xbt/dict.h>
#include <msg/msg.h>

/* Simbatch headers */
#include "simbatch_config.h"
#include "utils.h"
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

/* Type declaration */
typedef struct context_t {
  m_cluster_t cluster;
  plugin_scheduler_t scheduler;
  FILE *flog;
  FILE *fout;
} context_t;

/* Functions declaration */
int
get_task(xbt_fifo_t msg_stack);

void
handle_task(context_t self, xbt_fifo_t msg_stack, int *job_cpt);

void
schedule_task(context_t self, msg_task_t task, int *job_cpt);

void
reserve_slot(context_t self, msg_task_t task, int *job_cpt);

void
check_ACK(context_t self, msg_task_t task);

void
cancel_task(context_t self, msg_task_t task);

void
answer_to_DIET(context_t self);

void
predict_schedule(context_t self, msg_task_t task);

void
eval_HPF(context_t self, msg_task_t task);

void
init_pf(context_t self, msg_task_t task);


extern unsigned long int DIET_PARAM[2];
extern const char * DIET_FILE; 
extern int DIET_MODE;

double NOISE = 0.0;

/********** Here it starts *************/
int 
SB_batch(int argc, char *argv[])
{
  const char *workload_file, *parser_name;
  int job_cpt = 0;
  msg_process_t resource_manager = NULL;
  xbt_fifo_t msg_stack = NULL;
  context_t self;
    
#ifdef OUTPUT
  char *out_file;
  FILE *fout;
#endif  
    
    
#ifdef LOG
  /*************** Create log file ****************/
  FILE *flog = config_get_log_file(HOST_NAME());
#endif
    
  m_cluster_t cluster = NULL;
  pluginInfo_t plugin = NULL;
  plugin_scheduler_t scheduler = NULL;
  msg_process_t wld_process = NULL;
    
  /**************** Configuration ******************/  
    
  /*** Plug in ***/
  plugin = SB_request_plugin("init", sizeof(plugin_scheduler));
  if (!plugin) {
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
  if (!cluster) {
    simbatch_clean();
    xbt_die("Failed to create the cluster");
  }
#ifdef VERBOSE
  fprintf(stderr, "Number of nodes: %d\n", cluster->nb_nodes);
#endif
  MSG_host_set_data(MSG_host_self(), cluster);
    
  /*** External Load ***/
  workload_file = SB_request_external_load();
  parser_name = SB_request_parser();

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
  if (!fout) {
    fprintf(stderr,"failed\n");
  }
  fprintf(stderr,"%s\n", out_file);
#endif
    
#endif
    
    
  /****************** Create load ******************/
  if (workload_file && parser_name) {
    const char ** param = xbt_malloc(2 * sizeof(char *));
    param[0] = workload_file;
    param[1] = parser_name;

#ifdef VERBOSE
    fprintf(stderr, "Create load... \n");
#endif
    wld_process = MSG_process_create("external_load", SB_external_load,
				     (void *)param, MSG_host_self());
  }
#ifdef VERBOSE
  else {
    fprintf(stderr, "No load, dedicated platform\n");
  }
#endif
    
  /************* Create resource manager ************/
  resource_manager = MSG_process_create("Resource manager", 
					SB_resource_manager,
					NULL, MSG_host_self());	
    
  /************* start schedule ***************/  
#ifdef VERBOSE
  fprintf(stderr, "%s... ready\n", MSG_host_get_name(MSG_host_self())); 
#endif

#ifdef LOG
  fprintf(flog, "[%lf]\t%20s\tWait for tasks\n", MSG_get_clock(),
	  PROCESS_NAME());
#endif
    
  self.cluster = cluster;
  self.scheduler = scheduler;
#ifdef LOG
  self.flog = flog;
#endif
#ifdef OUTPUT
  self.fout = fout;
#endif
  
  char resource_manager_MB[256];
  sprintf(resource_manager_MB, "resource_manager-%s", HOST_NAME());
  msg_stack = xbt_fifo_new();
  /* Receiving messages and put them in a stack */
  while (1) {
    int ok = get_task(msg_stack);
    
    if (!ok) {
      break;
    } else {
      handle_task(self, msg_stack, &job_cpt);
    }
    
    MSG_task_send(MSG_task_create("SB_UPDATE", 0.0, 0.0, NULL), 
		  resource_manager_MB);
  }
    
  if (msg_stack) {
    xbt_fifo_free(msg_stack);
  }
    
  /* Clean */ 
#ifdef OUTPUT
  fclose(fout);
#ifdef VERBOSE
  fprintf(stderr, "Close %s... ok\n", out_file);
#endif
  free(out_file);
#endif

  cluster_destroy(&cluster);
#ifdef VERBOSE
  fprintf(stderr, "%s cleaned\n", HOST_NAME());
#endif
    
  return EXIT_SUCCESS;
}


int
get_task(xbt_fifo_t msg_stack)
{
  msg_task_t task = NULL;
  msg_error_t err;
  char name[256];

  sprintf(name, "batch-%s", HOST_NAME());
  
  err = MSG_task_receive_with_timeout(&task, name, DBL_MAX);	

  if (err == MSG_TIMEOUT) {
    return false;
  }
  
  xbt_fifo_push(msg_stack, task);
  
  while (MSG_task_listen(name)) {
    task = NULL;
    MSG_task_receive(&task, name);
    xbt_fifo_push(msg_stack, task);
  }
  
  xbt_fifo_alphabetically_sort(msg_stack);
  return true;
}


/*
 * Tasks are handled in their alphabetical order
 * PF_INIT
 * SB_ACK
 * SB_CANCEL
 * SB_DIET
 * SB_RES
 * SB_TASK
 * SED_HPF
 * SED PRED
 */
void
handle_task(context_t self, xbt_fifo_t msg_stack, int *job_cpt)
{
  /* retrieve messages from the stack */
  while (xbt_fifo_size(msg_stack)) {
    msg_task_t task = xbt_fifo_shift(msg_stack); 
        
    if (!strcmp(task->name, "SB_TASK")) {
      /* handle task. */
      schedule_task(self, task, job_cpt);
    } else if (!strcmp(task->name, "SB_RES")) {
      /* handle reservation. */
      reserve_slot(self, task, job_cpt);
    } else if (!strcmp(task->name, "SB_ACK")) {
      /* handle finished task. */
      check_ACK(self, task);
    } else if (!strcmp(task->name, "SB_CANCEL")) {
      /* handle cancel task, dangerous. */
      cancel_task(self, task);
    } else if (!strcmp(task->name, "SB_DIET")) {
      /* handle DIET request. */
      answer_to_DIET(self);
    } else if (!strcmp(task->name, "SED_PRED")) {
      /* handle prediction. */
      predict_schedule(self, task);
    } else if (!strcmp(task->name, "SED_HPF")) {
      /* handle HPF algo. Work in progress. */
      eval_HPF(self, task);
    } else if (!strcmp(task->name, "PF_INIT")) {
      /* get info for initializing. */
      init_pf(self, task);
    } else {
#ifdef VERBOSE
      fprintf(stderr, "Unknown task (%s) received on %s\n",
	      task->name, MSG_host_get_name(MSG_host_self()));
#endif
    }
    MSG_task_destroy(task);
#ifdef LOG
    fflush(self.flog);
#endif
  }
}


void
schedule_task(context_t self, msg_task_t task, int *job_cpt)
{
  job_t job = MSG_task_get_data(task);
  msg_host_t sender = MSG_task_get_source(task);
    
  job->id = (*job_cpt)++;
  
#ifdef LOG	
  fprintf(self.flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" for schedule\n",
	  MSG_get_clock(), PROCESS_NAME(), job->name,
	  MSG_host_get_name(sender));
#endif
  
  if (job->nb_procs <= self.cluster->nb_nodes && 
      (job->deadline == -1 ||
       job->deadline > MSG_get_clock())) {
    /* We keep a trace of the task */
    if (job->priority >= self.cluster->priority)
      job->priority = self.cluster->priority - 1;
        
    job->entry_time = MSG_get_clock();
    /* Noise */
    job->run_time += NOISE;
    if (job->mapping) {
      xbt_free(job->mapping);
    }
    job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
    xbt_dynar_push(self.cluster->queues[job->priority], &job); 
    
    /* ask to the plugin to schedule and accept this new task. */
    self.scheduler->accept(self.cluster, job,
			   self.scheduler->schedule(self.cluster, job));
  } else {
#ifdef LOG
    fprintf(self.flog, "[%lf]\t%20s\t%s canceled: not enough ressource\n",
	    MSG_get_clock(), PROCESS_NAME(), job->name);
#endif
  }    
}


void
reserve_slot(context_t self, msg_task_t task, int *job_cpt)
{
  job_t job = MSG_task_get_data(task);
  msg_host_t sender = MSG_task_get_source(task);
  slot_t * slot = NULL;
  int it = 0;
  char sender_MB[256];
  sprintf(sender_MB, "client-%s", MSG_host_get_name(sender));
    
  job->id = (*job_cpt)++;
	
#ifdef LOG
  fprintf(self.flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" for reservation\n",
	  MSG_get_clock(), PROCESS_NAME(), job->name, 
	  MSG_host_get_name(sender));
#endif
    
  slot = find_a_slot(self.cluster, job->nb_procs, job->start_time,
		     job->wall_time);
  fprintf(stdout, "Slot: \n");
  print_slot(slot, self.cluster->nb_nodes);
  /* check reservation validity */
  if (slot[job->nb_procs-1]->start_time == job->start_time) {
    job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
    xbt_dynar_push(self.cluster->reservations, &job);
    /* insert reservation into the Gantt chart */
    job->start_time = slot[0]->start_time;
    for (it = 0; it < job->nb_procs; ++it) {
      job->mapping[it] = slot[it]->node; 
      xbt_dynar_insert_at(self.cluster->waiting_queue[slot[it]->node], 
			  slot[it]->position, &job);
    }

    MSG_task_async_send(MSG_task_create("SB_OK", 0.0, 0.0, NULL),
			sender_MB);
  } else {
    fprintf(stdout, "Reservation impossible");
    xbt_free(slot);
    xbt_free(job);
    MSG_task_async_send(MSG_task_create("SB_KO", 0.0, 0.0, NULL),
			sender_MB);
  }
}


void
check_ACK(context_t self, msg_task_t task)
{
  job_t job = MSG_task_get_data(task);
  int it;
  
#ifdef LOG
  fprintf(self.flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" \n",
	  MSG_get_clock(), PROCESS_NAME(), task->name,
	  MSG_process_get_name(MSG_task_get_sender(task)));
  fprintf(self.flog, "[%lf]\t%20s\t%s done\n", 
	  MSG_get_clock(), PROCESS_NAME(), job->name);
#endif
  
  job->state = DONE;
  job->completion_time = MSG_get_clock();
  cluster_delete_done_job(self.cluster, job);
  
  it = cluster_search_job(self.cluster, job->id, self.cluster->reservations);
  if (it == -1) { 
    it = cluster_search_job(self.cluster, job->id, 
			    self.cluster->queues[job->priority]);
    xbt_dynar_remove_at(self.cluster->queues[job->priority], it, NULL);
  } else {
    xbt_dynar_remove_at(self.cluster->reservations, it, NULL);
  }
  
#ifdef OUTPUT
  if (job->state != ERROR) {   
    fprintf(self.fout, "%-15s\t%d\t%lf\t%lf\t%lf\t%lf\t\n", job->name,
	    job->nb_procs, job->submit_time, job->start_time + NOISE, 
	    job->completion_time, MSG_get_clock() - job->start_time - NOISE);
    fflush(self.fout);
  }
#endif    
  
  if (job->free_on_completion) {
    xbt_free(job->mapping);
    xbt_free(job);
  }
  
  /* The system becomes stable again, we can now reschedule */
  self.scheduler->reschedule(self.cluster, self.scheduler);
}


void
cancel_task(context_t self, msg_task_t task)
{
  job_t job = MSG_task_get_data(task);
  msg_host_t sender = MSG_task_get_source(task);
  int it;
  char sender_MB[256];
  sprintf(sender_MB, "client-%s", MSG_host_get_name(sender));
  
  if (job->state != WAITING) {
    MSG_task_send(MSG_task_create("CANCEL_KO", 0.0, 0.0, NULL),
		  sender_MB);
  } else {
    cluster_delete_done_job(self.cluster, job);
    it = cluster_search_job(self.cluster, job->id,
			    self.cluster->reservations);
    if (it == -1) { 
      it = cluster_search_job(self.cluster, job->id,
			      self.cluster->queues[job->priority]);
      xbt_dynar_remove_at(self.cluster->queues[job->priority], it, NULL);
    } else { 
      xbt_dynar_remove_at(self.cluster->reservations, it, NULL); 
    }
    
#ifdef LOG
    fprintf(self.flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" \n",
	    MSG_get_clock(), PROCESS_NAME(), task->name,
	    MSG_process_get_name(MSG_task_get_sender(task)));
    fprintf(self.flog, "[%lf]\t%20s\t%s cancelled\n", 
	    MSG_get_clock(), PROCESS_NAME(), job->name);
#endif

    /* The system becomes stable again, we can reschedule */
    self.scheduler->reschedule(self.cluster, self.scheduler);
    MSG_task_send(MSG_task_create("CANCEL_OK", 0.0, 0.0, NULL),
		 sender_MB);
  }
}


void
answer_to_DIET(context_t self)
{
  FILE * fdiet = fopen(DIET_FILE, "w"); 
  slot_t * slots = NULL;
    
  if (!fdiet) {
    DIET_MODE = 0;
#ifdef VERBOSE
    fprintf(stderr, "%s: %s\n", DIET_FILE, strerror(errno));
#endif
  }
    
  job_t job =  xbt_malloc(sizeof(*job));
  strcpy(job->name, "DIET");
  job->submit_time = MSG_get_clock();
  job->input_size = 0.0; 
  job->output_size = 0.0;
  job->priority = 0.0;         
    
  if (DIET_PARAM[1] > self.cluster->nb_nodes) 
    DIET_PARAM[1] = self.cluster->nb_nodes;
    
  job->nb_procs = DIET_PARAM[1];
  job->wall_time = DIET_PARAM[0];
  job->run_time = DIET_PARAM[0];
  job->mapping = xbt_malloc(job->nb_procs * sizeof(int));
    
  slots = self.scheduler->schedule(self.cluster, job);
  fprintf(fdiet, "[%lf] DIET answer : %lf\n", MSG_get_clock(),
	  slots[0]->start_time);
  xbt_free(slots), slots = NULL;
  xbt_free(job->mapping);
  xbt_free(job);
    
  fclose(fdiet);
}


void
predict_schedule(context_t self, msg_task_t task)
{
  job_t job = MSG_task_get_data(task);
  msg_host_t sender = MSG_task_get_source(task);
  slot_t * slots = NULL;
  char sender_MB[256];
  sprintf(sender_MB, "client-%s", MSG_host_get_name(sender));
  
#ifdef VERBOSE
  fprintf(stdout, "Prediction for %s on %s\n", job->name,
	  HOST_NAME());
#endif
    
#ifdef LOG
  fprintf(self.flog, "[%lf]\t%20s\tReceive \"%s\" from \"%s\" for prediction\n",
	  MSG_get_clock(), PROCESS_NAME(), job->name,
	  MSG_host_get_name(sender));
#endif

  if (job->nb_procs <= self.cluster->nb_nodes) {
    if (job->priority >= self.cluster->priority) {
      job->priority = self.cluster->priority - 1;
    }
    
    slots = self.scheduler->schedule(self.cluster, job);
    
    MSG_task_async_send(MSG_task_create("SB_PRED", 0.0, 0.0, slots),
		       sender_MB);
  } else {
    MSG_task_async_send(MSG_task_create("SB_CLUSTER_KO", 0.0, 0.0, NULL),
		       sender_MB);
  }
}


void
eval_HPF(context_t self, msg_task_t task)
{
  job_t job = MSG_task_get_data(task);
  msg_host_t sender = MSG_task_get_source(task);
  char sender_MB[256];
  double host_speed = 0;
  double * weight = xbt_malloc(sizeof(double));
  msg_task_t HPF_value = NULL;
  
  sprintf(sender_MB, "client-%s", MSG_host_get_name(sender));
  fprintf(stdout, "Heuristique\n");
    
  if (job->nb_procs > self.cluster->nb_nodes) {
    printf("SB_CLUSTER_KO\n");
    *weight = 0;
    HPF_value = MSG_task_create("SB_CLUSTER_KO", 0.0, 0.0, weight);
  } else {
    slot_t * slots = self.scheduler->schedule(self.cluster, job);
    double waitT = (slots[0]->start_time - MSG_get_clock())?: 1;
        
    host_speed = MSG_get_host_speed(MSG_host_self());
    *weight = (self.cluster->nb_nodes * host_speed) / waitT;
    HPF_value = MSG_task_create("SB_HPF", 0.0, 0.0, weight);
    xbt_free(slots);
    slots = NULL;
  }
  MSG_task_async_send(HPF_value, sender_MB);
}


void
init_pf(context_t self, msg_task_t task)
{
  msg_host_t sender = MSG_task_get_source(task);
  int * answer = xbt_malloc(sizeof(int));
  char sender_MB [256];
  
  sprintf(sender_MB, "client-%s", MSG_host_get_name(sender));
  *answer = self.cluster->nb_nodes;
  MSG_task_async_send(MSG_task_create("SB_INIT", 0.0, 0.0, answer),
		     sender_MB);
}
