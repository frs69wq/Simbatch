#ifndef _SCHEDULING_H_
#define _SCHEDULING_H_

#include <xbt/dynar.h>
#include "cluster.h"
#include "plugin_scheduler.h"


/****************** Generic rescheduling functions ********************/
/* No re-schedule when a task just comes to end */
// void generic_no_reschedule(cluster_t cluster, scheduling_fct schedule);
void generic_no_reschedule(cluster_t cluster, plugin_scheduler_t scheduler);

/* Function to reschedule jobs waiting in the system */
// void generic_reschedule (cluster_t cluster, scheduling_fct schedule);
void generic_reschedule(cluster_t cluster, plugin_scheduler * scheduler);

/********************* Slot management functions *********************/
slot_t new_slot(int node_id, int position, double start_time, double duration);

int comp(const void * slot1, const void * slot2);

int check_matching_slot(slot_t * slots, int nb_nodes);

slot_t search_a_slot(cluster_t cluster, int node_id, int it,
		     double start_time, double end_time, double duration);

slot_t * find_a_slot(cluster_t cluster, int nb_nodes,
		     double start_time, double duration);


/* Return the next job to schedule in the queue*/
job_t next_job_to_schedule_in_queue(xbt_dynar_t queue);

/* Return the next job to schedule */
job_t next_job_to_schedule(cluster_t cluster);


/* usefull to build xbt_dynar of bids (xbt dynar request) */
__inline__ void free_slot(void * d);

/* print a xbt dynar of slots */
#ifdef DEBUG2
__inline__ void print_slots(xbt_dynar_t slots);
#endif

/* return the last slot of queue i */
slot_t get_last_slot(cluster_t cluster, int i);

/* return the n best slots sorted by start time (no check for duration) */
slot_t * select_n_slots(cluster_t cluster, xbt_dynar_t slots, int nb);

/* return the last completion time (end of work) */
double get_completion_time(cluster_t cluster);

#ifdef DEBUG2
__inline__ void print_slot(slot_t * slots, int size);
#endif


#endif
