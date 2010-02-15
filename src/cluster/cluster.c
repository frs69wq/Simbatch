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
#include <limits.h>
#include <errno.h>

#include <xbt/log.h>
#include <xbt/asserts.h>
#include <xbt/dynar.h>
#include <xbt/sysdep.h>

#include <msg/msg.h>

#include "simbatch_config.h"
#include "job.h"
#include "cluster.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cluster,"Logging for the simulator scheduler");
 

/******** Private functions **********/
/* Create a new cluster */
static
m_cluster_t cluster_new(int argc, char **argv, int nb);

static m_cluster_t
cluster_new(int argc, char **argv, int nb)
{
    m_cluster_t cluster;
    int i;
    
    /* Process organisation */
    /* no nodes... no cluster! */ 
    if (argc < 2) {
        INFO0("No nodes are defined!");
        abort();
    }
    
    cluster = xbt_malloc(sizeof(*cluster));
    cluster->nb_nodes = argc - 1;
    cluster->priority = nb;
    cluster->nodes = xbt_malloc(cluster->nb_nodes * sizeof(*(cluster->nodes)));
    cluster->waiting_queue = xbt_malloc(cluster->nb_nodes * 
					sizeof(*(cluster->waiting_queue)));
    
    for (i=0; i<argc-1; ++i) {
	cluster->waiting_queue[i] = xbt_dynar_new(sizeof(job_t), NULL);  
	cluster->nodes[i] = MSG_get_host_by_name(argv[i+1]);
        if (!cluster->nodes[i]) {
            INFO1("Unknown host %s. Stopping Now! ", argv[i+1]);
            xbt_abort();
        }
    }
    
    cluster->queues = xbt_malloc(cluster->nb_nodes * 
				 sizeof(*(cluster->queues)));
    
    for (i=0; i<cluster->priority; ++i) 
	cluster->queues[i] = xbt_dynar_new(sizeof(job_t), NULL);
    cluster->reservations = xbt_dynar_new(sizeof(job_t), NULL);

    return cluster;
}


/*********** Public functions **********/

m_cluster_t
SB_request_cluster(int argc, char **argv)
{
    m_cluster_t cluster = NULL;
    char request[128];
    const char * data;
    int nbQueue;
    
  /*** Cluster ***/
#ifdef VERBOSE
    fprintf(stderr, "Cluster init...\t");
#endif
    sprintf(request, 
	    "/config/batch[@host=\"%s\"]/priority_queue/number/text()",\
	    MSG_host_get_name(MSG_host_self())); 
    
    data = config_get_value(request);
    
    if (data == NULL) {
#ifdef VERBOSE
        fprintf(stderr, "failed\n");
        fprintf(stderr, "XPathError : %s\n", request);
#endif
	return NULL; 
    }
    nbQueue =  strtol(data, NULL, 10);
    
    cluster = cluster_new(argc, argv, nbQueue);
    if (cluster == NULL) {
#ifdef VERBOSE 
        fprintf(stderr, "failed\n");
        fprintf(stderr, "No nodes are defined for this cluster!\n");
#endif
        return NULL;
    }
    
#ifdef VERBOSE
    fprintf(stderr, "ok\n");
    fprintf(stderr, "Nb of priority queues : %u\n", nbQueue);
#endif  
    
    return cluster;
}


void
cluster_destroy(m_cluster_t  *cluster)
{
    int i;
    
    xbt_dynar_free(&((*cluster))->reservations);
    
    for (i=0; i<(*cluster)->nb_nodes; ++i) 
	xbt_dynar_free(&((*cluster)->waiting_queue[i]));
    
    for (i=0; i<(*cluster)->priority; ++i) 
	xbt_dynar_free(&((*cluster)->queues[i]));
    
    xbt_free((*cluster)->queues);
    xbt_free((*cluster)->waiting_queue);
    xbt_free((*cluster)->nodes);
    xbt_free((*cluster));
}


void
cluster_clean(m_cluster_t cluster)
{
    int i = 0;
    
    /* Clean the the scheduling table */
    for (i=0; i<cluster->nb_nodes; i++) {
        
        int j = 0;
        while (j < xbt_dynar_length(cluster->waiting_queue[i])) {
            job_t * job = NULL;  
	    
            job = xbt_dynar_get_ptr(cluster->waiting_queue[i], j);
            if ((*job)->state == WAITING) {
                job_t del;     
                xbt_dynar_remove_at(cluster->waiting_queue[i], j, &del);
            }
            else 
                ++j; 
        }
    }
}


int
cluster_search_job(m_cluster_t cluster, int job_id, xbt_dynar_t dynar)
{
    unsigned int cpt = 0;
    job_t job = NULL;
    
    xbt_dynar_foreach(dynar, cpt, job) {   
        if (job->id == job_id)
            return cpt;
    }
    return -1;
}


void
cluster_print(m_cluster_t cluster)
{
  int i = 0, size = 0;
  
  printf("[%lf]Affichage\n", MSG_get_clock());
  for (i=0; i<cluster->nb_nodes; ++i) {
    unsigned int cpt;
    job_t job;
    
    printf("\n****** %s => %lu tasks ******\n", cluster->nodes[i]->name,
	   xbt_dynar_length(cluster->waiting_queue[i]));
    
    xbt_dynar_foreach(cluster->waiting_queue[i], cpt, job) {
      (job->state==PROCESSING)? 
	printf("*%s ", job->name): printf("%s ", job->name);
    }
  }
  printf("\n");
  
  for (i = 0; i < cluster->priority; ++i) 
    size += xbt_dynar_length(cluster->queues[i]);
  
  printf("\n****** bag => %d tasks ******\n\n", size);
}


void
cluster_delete_done_job(m_cluster_t cluster, job_t job) {
    int i;
    
    for (i=0; i<cluster->nb_nodes; ++i) {
        job_t  * j = NULL;	
        if (xbt_dynar_length(cluster->waiting_queue[i]) > 0) {
            j = (job_t *)xbt_dynar_get_ptr(cluster->waiting_queue[i], 0);
            if (job->id == (*j)->id)		
                xbt_dynar_remove_at (cluster->waiting_queue[i], 0, NULL);
        }
    }
}

/*
int 
 get_nb_queues(char *value)
 {
    int nb = 0;
    
    if (!value)
	return 0;
    
    nb = strtol(value, NULL, 0);
    if (((nb == LONG_MAX) || (nb == LONG_MIN)) && (errno == ERANGE))
	return 0;
    
    return nb;
}
*/
