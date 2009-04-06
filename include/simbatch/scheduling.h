/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _SCHEDULING_H_
#define _SCHEDULING_H_

#include <xbt/dynar.h>
#include "cluster.h"
#include "plugin_scheduler.h"


/****************** Generic rescheduling functions ********************/
/* No re-schedule when a task just comes to end */
// void generic_no_reschedule(m_cluster_t cluster, scheduling_fct schedule);
void
generic_no_reschedule(m_cluster_t cluster, plugin_scheduler_t scheduler);

/* Function to reschedule jobs waiting in the system */
// void generic_reschedule (m_cluster_t cluster, scheduling_fct schedule);
void
generic_reschedule(m_cluster_t cluster, plugin_scheduler *scheduler);

/********************* Slot management functions *********************/
slot_t
new_slot(int node_id, int position, double start_time, double duration);

int
comp(const void *slot1, const void *slot2);

int
check_matching_slot(slot_t *slots, int nb_nodes);

slot_t search_a_slot(m_cluster_t cluster, int node_id, int it,
		     double start_time, double end_time, double duration);

slot_t *
find_a_slot(m_cluster_t cluster, int nb_nodes, double start_time,
            double duration);


/* Return the next job to schedule in the queue*/
job_t
next_job_to_schedule_in_queue(xbt_dynar_t queue);

/* Return the next job to schedule */
job_t
next_job_to_schedule(m_cluster_t cluster);


/* usefull to build xbt_dynar of bids (xbt dynar request) */
void
free_slot(void *d);

/* print a xbt dynar of slots */
__inline__ void
print_slots(xbt_dynar_t slots);

/* return the last slot of queue i */
slot_t
get_last_slot(m_cluster_t cluster, int i);

/* return the n best slots sorted by start time (no check for duration) */
slot_t *
select_n_slots(m_cluster_t cluster, xbt_dynar_t slots, int nb);

/* return the last completion time (end of work) */
double
get_completion_time(m_cluster_t cluster);

__inline__ void
print_slot(slot_t *slots, int size);

#endif
