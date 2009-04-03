/****************************************************************************/
/* This file is part of the Simbatch project.                               */
/* written by Jean-Sebastien Gay and Ghislain Charrier, ENS Lyon.           */
/*                                                                          */
/* Copyright (c) 2007, Simbatch team. All rights reserved.                  */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/

/**
 * \file cluster.h
 * Define the cluster MSG_process.
 */

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include <xbt/fifo.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "job.h"

/**
 * Cluster definition.
 * This is a representation of the cluster for the SB_bacth process.
 * \todo typedef cluster_t {...} cluster_t, *m_cluster_t;
 */
typedef struct {
    int nb_nodes;   /*<! Number of slaves. */
    int priority;   /*<! Number of priority level available for a task. */
    m_host_t *nodes;    /*<! List of slaves. */
    xbt_dynar_t *queues;    /*<! Trace the tasks (batch + cluster). */
    xbt_dynar_t *waiting_queue; /*<! Scheduling table or Gantt diagramm. */
    xbt_dynar_t reservations;   /*<! Reservation queue. */ 
} *cluster_t;


/**
 * Cluster constructor.
 * The cluster representation is built with the parameters defined in the
 * deployment.xml files.
 * \param argc number of parameters.
 * \param **argv list of parameters.
 * \return a pointer on the cluster structure.
 */
cluster_t
SB_request_cluster(int argc, char **argv);

/**
 * Free a cluster structure. 
 * \param *cluster the cluster structure to be freed.
 */ 
void
cluster_destroy(cluster_t  *cluster);     

/**
 * Remove jobs in WAITING state (waiting) from queues.
 * \param cluster the cluster containing the queues. 
 */
void
cluster_clean(cluster_t cluster);

/**
 * Search a job by its id.
 * \param cluster deprecated parameter 
 * \param job_id identifiant of the searched job.
 * \param dynar structure containing the job.
 * \return -1 if the search failed, index of the job_id in dynar otherwise.
 */
int 
cluster_search_job(cluster_t cluster, int job_id, xbt_dynar_t dynar);

/**
 * Delete a job in the cluster.
 * This function is called when the job becomes done. All the SB_ACK have been
 * been received. So The job should be at the first place in the scheduling
 * table as it is in a running state.
 * \param cluster the cluster repreentation.
 * \param job the job to delete from the scheduling table.
 */
void 
cluster_delete_done_job(cluster_t cluster, job_t job);

/** 
 * Print some informations on a cluster.
 * Used as a debug function.
 * \param the cluster representation. 
 */
void 
cluster_print(cluster_t cluster);

/*
 * \return return the number of priority queue for the cluster 
 * 
 */
/* int get_nb_queues(char * value); */

#endif 
