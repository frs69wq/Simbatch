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

#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "plugin_scheduler.h"
#include "job.h"
#include "cluster.h"
#include "scheduling.h"

/************* Functions *************/
static slot_t *
fcfs_schedule(m_cluster_t cluster, job_t job);

static void
fcfs_accept(m_cluster_t cluster, job_t job, slot_t *slots);

/**************** Code ***************/
plugin_scheduler_t
init (plugin_scheduler_t p)
{
    p->schedule = fcfs_schedule;
    p->reschedule = generic_reschedule;
    p->accept = fcfs_accept;
  
    return p;
}


static slot_t *
fcfs_schedule(m_cluster_t cluster, job_t job)
{
    int i = 0;
    slot_t *best_slots = NULL;
    xbt_dynar_t slots = NULL;
    
    slots = xbt_dynar_new(sizeof(slot_t), free_slot);
    
    /* We take the slot for each queue */
    for (i=0; i<cluster->nb_nodes; i++) {
        slot_t s;
        s = get_last_slot(cluster,i);
        xbt_dynar_push(slots, &s);
    }
    
    /* We select the n best bids (the lowest date) */
    best_slots = select_n_slots(cluster, slots, job->nb_procs);

    /* cleaning every strtuctures */
    xbt_dynar_free(&slots);

    return best_slots;
}

static void
fcfs_accept(m_cluster_t cluster, job_t job, slot_t *slots)
{
    int i=0;
    
    /* The start_time of the job is finally equal to the
       worst start_time among the best */
    job->start_time = slots[job->nb_procs-1]->start_time;
    
    /* we must push the task in its waiting queue(s) */
    for (i=0; i<job->nb_procs; i++) {
        job->mapping[i] = slots[i]->node;
        xbt_dynar_push(cluster->waiting_queue[slots[i]->node], &job);
    }

    job->completion_time = job->start_time + job->wall_time;

    for (i=0;i<job->nb_procs;i++)
        xbt_free(slots[i]);
    xbt_free(slots);
}
 
