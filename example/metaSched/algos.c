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
#include <float.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include <msg/msg.h>

#include "simbatch.h"
#include "algos.h"


winner_t * MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job) {
    int i = 0;
    double completionT = DBL_MAX;
    winner_t * winner = xbt_malloc(sizeof(winner_t));
    int failure = 0;

    winner->completionT = DBL_MAX;
    winner->job = job;
    
   
    for (i=0; i<nbClusters; ++i) {
        m_task_t task = NULL;
      
        MSG_task_put(MSG_task_create("SED_PRED", 0, 0, job), clusters[i], SED_CHANNEL);
        MSG_task_get(&task, MS_CHANNEL);
        if (!strcmp(task->name, "SB_PRED")) { // OK
            slot_t * slots = NULL;
            slots = MSG_task_get_data(task);
            //print_slot(slots, 5);
            completionT = slots[0]->start_time + (job->wall_time * speedCoef[i]);
            if (completionT < winner->completionT) { 
                winner->completionT = completionT;
                winner->cluster = slots[0]->host;
            }
            xbt_free(slots);
        }
        else { 
            const m_host_t sender = MSG_task_get_source(task);
            if (!strcmp(task->name, "SB_SERVICE_KO"))
                printf("Service unavailable on host: %s\n", sender->name);
            else
                printf("Unadequate resources: %s\n", sender->name);

            ++failure;
        }
       
        printf("\n");
        MSG_task_destroy(task), task = NULL;
    }

    if (failure == nbClusters) { winner->completionT = -1; }

    return winner;
}


p_winner_t
MinMin_schedule(const m_host_t * clusters, const int nbClusters, 
                const double * speedCoef, xbt_fifo_t bagofJobs) {
    p_winner_t winners[xbt_fifo_size(bagofJobs)];
    p_winner_t bigWinner = NULL;
    xbt_fifo_item_t bucket = NULL;
    job_t job = NULL;
    int i = 0;
    
    // foreach job select min MCT
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) {
        winners[i++] = MCT_schedule(clusters, nbClusters, speedCoef, job);
    }
    
    // foreach MCT estimation, select the min
    bigWinner = winners[0];
    for (i=1; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i]->completionT > 0) {
            if (bigWinner->completionT < winners[i]->completionT) {
                bigWinner = winners[i]; 
            } 
        }
    }
    
    // free loosers
    for (i=0; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i] != bigWinner) { xbt_free(winners[i]); }
    }
    
    return bigWinner;
}

 
p_winner_t
MaxMin_schedule(const m_host_t * clusters, const int nbClusters, 
                const double * speedCoef, xbt_fifo_t bagofJobs) {
    p_winner_t winners[xbt_fifo_size(bagofJobs)];
    p_winner_t bigWinner = NULL;
    xbt_fifo_item_t bucket = NULL;
    job_t job = NULL;
    int i = 0;
    
    // foreach job select min MCT
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) {
        winners[i++] = MCT_schedule(clusters, nbClusters, speedCoef, job);
    }
    
    // foreach MCT estimation, select the min
    bigWinner = winners[0];
    for (i=1; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i]->completionT > 0) {
            if (bigWinner->completionT > winners[i]->completionT) {
                bigWinner = winners[i]; 
            } 
        }
    }
    
    // free loosers
    for (i=0; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i] != bigWinner) { xbt_free(winners[i]); }
    }

    return bigWinner;
}


void HPF(const m_host_t * clusters, const int nbClusters, 
         const double * speedCoef, job_t job, 
         const int * nbNodesPF, const int nbNodesTot) {
    double * data = 0;
    int nbServiceOk = nbNodesTot;
    int i = 0;
    
    job->weight = 0;
     
    for (i=0; i<nbClusters; ++i) {
        m_task_t task = NULL;
        
        MSG_task_put(MSG_task_create("SED_HPF", 0, 0, job), clusters[i], SED_CHANNEL);
        MSG_task_get(&task, MS_CHANNEL);
        if (!strcmp(task->name, "SB_SERVICE_KO")) { nbServiceOk -= nbNodesPF[i]; }
        // else if (!strcmp(task->name, "SB_CLUSTER_KO")) { }
        else {
            data = (double *)MSG_task_get_data(task);
            job->weight += *data;
            xbt_free(data);
        }
        MSG_task_destroy(task);
        task = NULL;
    }
    
    if (job->weight != 0)  {job->weight = 10000000/(double)(job->weight); } 
    printf ("weight : %lf \t representativite : %d/%d\n", 
            job->weight, nbNodesTot, nbServiceOk);
    job->weight *= (nbServiceOk>0)? nbNodesTot/nbServiceOk: 0;
}


p_winner_t
HPF_schedule(const m_host_t * clusters, const int nbClusters, 
             const double * speedCoef, xbt_fifo_t bagofJobs,
             const int * nbNodesPF, const int nbNodesTot) {
    xbt_fifo_item_t bucket = NULL;
    job_t job;
    job_t criticalJob = NULL;
    
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) { 
        HPF(clusters, nbClusters, speedCoef, job, nbNodesPF, nbNodesTot);
        printf ("weight %s: %lf\n", job->name, job->weight); 
        if (!criticalJob) { criticalJob = job; }
        else if (criticalJob->weight < job->weight) { criticalJob = job; }
    }

    return MCT_schedule(clusters, nbClusters, speedCoef, criticalJob);
}