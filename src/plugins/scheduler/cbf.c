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
static slot_t * cbf_schedule(cluster_t cluster, job_t job);
static void cbf_accept(cluster_t cluster, job_t job, slot_t * slots);

/**************** Code ***************/
plugin_scheduler_t init (plugin_scheduler_t p) {
    p->schedule = cbf_schedule;
    p->reschedule = generic_reschedule;
    p->accept = cbf_accept;
    
    return p;
}


static slot_t * cbf_schedule(cluster_t cluster, job_t job) {
    slot_t  * slots;
    
    slots = find_a_slot(cluster, job->nb_procs,
			MSG_get_clock(), job->wall_time);
 
    return slots;
}


static void cbf_accept(cluster_t cluster, job_t job, slot_t * slots) {
    int i = 0;
    job->start_time = slots[0]->start_time;
    for (i=0; i<job->nb_procs; ++i) {
	job->mapping[i] = slots[i]->node; 
	xbt_dynar_insert_at(cluster->waiting_queue[slots[i]->node], 
			    slots[i]->position, &job);
    }
    job->completion_time = slots[0]->start_time + slots[0]->duration;    
    xbt_free(slots);
}
