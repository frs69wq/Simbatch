/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "job.h"
/*
 * Cluster definition
 * - Number of slaves
 * - Number of priority's level
 * - The slaves
 * - Trace of the tasks actually in the system (batch + cluster)
 * - Scheduling table
 * - Reservation queue
 */
typedef struct {
    int nb_nodes;
    int priority;
    m_host_t * nodes;
    xbt_dynar_t * queues;
    xbt_dynar_t * waiting_queue;
    xbt_dynar_t reservations;
} * cluster_t;


/* Request a cluster with the params defined in the xml files */
cluster_t  SB_request_cluster(int argc, char ** argv);

/* Free a cluster */ 
void cluster_destroy(cluster_t  * cluster);     

/* Remove only job that are in WAITING state (waiting) */
void cluster_clean(cluster_t cluster);

/* Search a job by its id */
int cluster_search_job(cluster_t cluster, int job_id, xbt_dynar_t dynar);

/* Delete a job in the cluster (job should be at first place (running)*/
void cluster_delete_done_job(cluster_t cluster, job_t job);

/* Prints some infos on a cluster */
void cluster_print(cluster_t cluster);

/* return the number of priority queue for the cluster 
 * Unused Function in the project
 * 
 */
/* int get_nb_queues(char * value);*/

#endif 
