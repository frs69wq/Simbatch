/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


/********************************
 *
 * CBF : Conservative BackFilling
 *
 ********************************/

#include <stdio.h>
#include <stdlib.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "cluster.h"
#include "job.h"
#include "scheduling.h"
#include "plugin_scheduler.h"

/************* Functions *************/
static slot_t *
cbf_schedule(m_cluster_t cluster, job_t job);

static void
cbf_accept(m_cluster_t cluster, job_t job, slot_t *slots);

/**************** Code ***************/
plugin_scheduler_t
init(plugin_scheduler_t p)
{
    p->schedule = cbf_schedule;
    p->reschedule = generic_reschedule;
    p->accept = cbf_accept;
    
    return p;
}


static slot_t *
cbf_schedule(m_cluster_t cluster, job_t job)
{
    slot_t * slots;
    slot_t * best_slots;
    int i;
    slots = find_a_slot(cluster, job->nb_procs, MSG_get_clock(),
                        job->wall_time);
    best_slots = xbt_malloc(job->nb_procs * sizeof(slot_t));
    for (i = 0; i < cluster->nb_nodes; i++) {
      if (i < job->nb_procs) {
	best_slots[i] = slots[i];
      }
      else {
	xbt_free(slots[i]);
      }
    }
    xbt_free(slots);

    return best_slots;
}


static void
cbf_accept(m_cluster_t cluster, job_t job, slot_t *slots)
{
    int i = 0;
    
    job->start_time = slots[0]->start_time;
    for (i=0; i<job->nb_procs; ++i) {
        job->mapping[i] = slots[i]->node; 
        xbt_dynar_insert_at(cluster->waiting_queue[slots[i]->node], 
                            slots[i]->position, &job);
    }
    job->completion_time = job->start_time + job->wall_time;    
    
    for (i=0;i<job->nb_procs;i++)
      xbt_free(slots[i]);
    xbt_free(slots);
}
