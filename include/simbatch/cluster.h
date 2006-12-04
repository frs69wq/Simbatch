/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "job.h"
/*
 * Cluster definition
 */
typedef struct {
  /* Number of slaves */ 
  int nb_nodes;
  /* Number of priority's level */
  int priority;
  /* Here are my slaves */ 
  m_host_t * nodes;
  /* Trace of the tasks actually in the system (batch + cluster) */
  xbt_dynar_t * queues;
  /* Scheduling table */
  xbt_dynar_t * waiting_queue;
} * cluster_t;


/* Request a cluster with the params defined in the xml files */
cluster_t  SB_request_cluster(int argc, char ** argv);

/* Free a cluster */ 
void cluster_destroy(cluster_t  * cluster);     

/* Remove only job that are in WAITING state (waiting) */
void cluster_clean(cluster_t cluster);

/* Search a job by its id */
int cluster_search_job(cluster_t cluster, int job_id, int priority);

/* Delete a job in the cluster (job should be at first place (running)*/
void cluster_delete_done_job(cluster_t cluster, job_t job);

#ifdef GANTT
void cluster_print(cluster_t cluster);
#endif

/* return the number of priority queue for the cluster */
int get_nb_queues(char * value);

#endif 
