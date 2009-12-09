/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <xbt/fifo.h>
#include <xbt/ex.h>
#include <xbt/sysdep.h>
#include <msg/msg.h>

#include "simbatch.h"
#include "simbatch/plugin.h"
#include "simbatch/plugin_input.h"
#include "algos.h"

/*
 * prints the args of a function
 */
void printArgs(int argc, char ** argv) {
  int i=1;
  for (i=1; i<argc; ++i) {
    printf("\t%2d - %s\n", i, argv[i]); 
  }
}

/*
 * Transforms a string into a double with some verifications
 */
double str2double(const char * str) {
    char * end;
    double val = strtod(str, &end);

    if (end == str) { return 0; }
    if (((val == DBL_MAX) || (val == DBL_MIN)) && (errno == ERANGE)) {
        return 0;
    }
    return val;
}

/*
 * Checks if a service is availible
 */
int isAvailable(const int services[], const  int service) {
  int i = 0;
  while (services[i]!=-1) {
    if (services[i] == service) {
      return 1;
    }
    i++;
  }
  return 0;
}

/*
 * Speed ratio between clusters to have the fastest one
 */
double * getSpeedCoef(const m_host_t * clusters, const int nbClusters) {
  double * speedCoef = NULL;
  double maxSpeed = 0;
  int i=0;

  speedCoef = xbt_malloc(sizeof(double) * nbClusters);

  //get the fastest speed
  for (i=0; i<nbClusters; ++i) {
    if (maxSpeed < MSG_get_host_speed(clusters[i]))
      maxSpeed = MSG_get_host_speed(clusters[i]);
  }

  //computes speed coefficients of each cluster
  for (i=0; i<nbClusters; ++i) {
    speedCoef[i] = maxSpeed / MSG_get_host_speed(clusters[i]);
  }

  return speedCoef;
}


/*
 * Makes a first call to the batch to obtian infos such as
 * the number of nodes on the cluster
 */
int * getNbNodesPF(const m_host_t * clusters, const int nbClusters) {
  int i = 0;
  int * clusterInfo = xbt_malloc(sizeof(int) * nbClusters);
  m_task_t task = NULL;
  int * data;
  char cluster_MB[256];
  char name[256] = "metasched_MB";
  /* For each cluster, we ask the number of nodes */
  for (i=0; i<nbClusters; ++i) {
    sprintf(cluster_MB, "client-%s", MSG_host_get_name(clusters[i]));
    MSG_task_send(MSG_task_create("PF_INIT", 0, 0, NULL), cluster_MB);
    MSG_task_receive_with_timeout(&task, name, DBL_MAX);
    //MSG_task_receive(&task, name);
    data = (int *)MSG_task_get_data(task);
    clusterInfo[i] = *data;
    xbt_free(data);
    MSG_task_destroy(task);
    task = NULL;
  }
    
  return clusterInfo;
}



/* 
 * Choose which batch executes a task using the MCT algorithm
 */
int metaSched(int argc, char ** argv) {
  const int nbClusters = argc - 1;
  int * nbNodesPF;
  m_host_t clusters[nbClusters];
  xbt_fifo_t jobList = NULL;
  p_winner_t winner = NULL;
  double * speedCoef = NULL;
  int nbNodesTot = 0;
  int i = 1;
  void *handle = NULL;
  plugin_input plugin;

  /* prints infos on the batch */
  /*printf("%s - ", MSG_host_get_name(MSG_host_self()));
  printf("%s\n", MSG_process_get_name(MSG_process_self()));
  printArgs(argc, argv);*/

  for (i=1; i<argc; ++i) {
    clusters[i-1] = MSG_get_host_by_name(argv[i]);
  }
  
  speedCoef = getSpeedCoef(clusters, nbClusters);
  
  nbNodesPF = getNbNodesPF(clusters, nbClusters);
  for (i=0; i<nbClusters; ++i) { nbNodesTot += nbNodesPF[i]; }
  
  //plugin to parse the file containing the jobs
  launch_plugin(&handle, &plugin, "../../lib/libwld.so", "init_input");
  jobList = plugin.create_list("./load.wld", "job");
  close_plugin(handle);
  
  double time = 0;
  job_t job;
  char sed_MB[256];
  /* for each job, choose the cluster where the job will be 
     completed first */
  while ((job=(job_t)xbt_fifo_shift(jobList)) != NULL) { 
    winner = MCT_schedule(clusters, nbClusters, speedCoef, job);
    /* If there is a winner, send the task to this cluster */
    if (winner->completionT != -1) {
      printf("Winner for job %lu is %s!\n", job->user_id, 
	     winner->cluster->name);
      MSG_process_sleep(job->submit_time - time);
      sprintf(sed_MB, "client-%s", winner->cluster->name);
      MSG_task_send(MSG_task_create("SB_TASK", 0, 0, job), 
		    sed_MB);
      time = job->submit_time;
    }
    else {
      printf("no winner for job %lu!\n", job->user_id); 
    }
    xbt_free(winner);
  }

  xbt_fifo_free(jobList);
  xbt_free(speedCoef);
  xbt_free(nbNodesPF);
  
  return EXIT_SUCCESS;
}


/*
 * Represents an agent between the metaScheduler and the Batch
 * It is responsible for communications between these two agents
 */
int sed(int argc, char ** argv) {
  const m_host_t batch = MSG_get_host_by_name(argv[2]);
  const int nbServices = argc - 4;
  //const double waitT = str2double(argv[3]);
  int services[nbServices + 1];
  xbt_fifo_t msg_stack = xbt_fifo_new();
  int i = 0;
  MSG_error_t err;
  m_task_t task = NULL;
  char name[256];
  sprintf(name, "client-%s", HOST_NAME());
  char batch_MB[256];
  sprintf(batch_MB, "batch-%s", HOST_NAME());
  
  /* Print the args of the sed */
  /*printf("%s - ", MSG_host_get_name(MSG_host_self()));
  printf("%s - %lf\n", MSG_process_get_name(MSG_process_self()), waitT);
  printArgs(argc, argv);*/

  /* Services availible */
  services[nbServices] = -1;
  for (i=0; i<nbServices; ++i) { services[i] = atoi(argv[4+i]); }
  /*for (i=0; services[i]!=-1; ++i) { printf("Service%d\t", services[i]); }
    printf("\n");*/

  while (1) {
    task = NULL;
    err = MSG_task_receive_with_timeout(&task, name, DBL_MAX);
    if (err == MSG_TIMEOUT_FAILURE) { 
      break;
    }

    xbt_fifo_push(msg_stack, task);
    while (MSG_task_listen(name)) {
      task = NULL;
      MSG_task_receive(&task, name);
      xbt_fifo_push(msg_stack, task);
    } 
    
    while (xbt_fifo_size(msg_stack)) {
      task = xbt_fifo_shift(msg_stack);
      /* If the task comes from the batch, it has to be transfered
       to the metaScheduler */
      if (MSG_task_get_source(task) == batch) {
	MSG_task_async_send(task, "metasched_MB"); 
	continue;
      }
      job_t job = MSG_task_get_data(task);
      
      if (!job) {
	/* Forward to the Batch */
	MSG_task_async_send(task, batch_MB);
	continue;
      }
      
      if (isAvailable(services, job->service)) {
	/* Forward to the Batch to execute the task or to make
           some prediction */
	MSG_task_async_send(task, batch_MB);
      }
      else { /* Service unavailable */
	printf("%s Service unavailable %s \n", 
	       HOST_NAME(), task->name);
	MSG_task_async_send(MSG_task_create("SB_SERVICE_KO", 0, 0, job), 
			   "metasched_MB"); 
      }
    }
  }
  xbt_fifo_free(msg_stack);
    
  return EXIT_SUCCESS;
}



int main(int argc, char ** argv) {
  /* initialisation */
  SB_global_init(&argc, argv);
  MSG_global_init(&argc, argv);
  
  /* Register functions (scheduler, computation, batch and node) */
  MSG_function_register("metaSched", metaSched);
  MSG_function_register("sed", sed);
  MSG_function_register("SB_batch", SB_batch);
  MSG_function_register("SB_node", SB_node);

  MSG_create_environment(SB_get_platform_file());
  MSG_launch_application(SB_get_deployment_file()); 

  MSG_main();

  SB_clean();
  MSG_clean();

  return EXIT_SUCCESS;
}
