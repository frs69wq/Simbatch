#include <stdio.h>
#include <stdlib.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "job.h"
#include "cluster.h"
#include "scheduling.h"


/* no reschedule: generic function 
 * (i.e. works for all scheduing algorithm) 
 */ 
void generic_no_reschedule (cluster_t cluster, plugin_scheduler_t scheduler) {
    return ;
}


void generic_reschedule (cluster_t cluster, plugin_scheduler_t scheduler) {
    auto void reschedule(job_t job);
    void reschedule(job_t job) { 
        if ((job->state) == WAITING) { 
            scheduler->accept(cluster, job, scheduler->schedule(cluster, job));
        }
    }

    int i = 0;

    /* remove waiting jobs */
    cluster_clean(cluster);
    
    /* Reschedule the tasks (just those tat are not processing) */
    for (i = cluster->priority - 1; i>=0; i--) {
	int cpt;
	job_t job = NULL;
	
	xbt_dynar_foreach(cluster->queues[i], cpt, job) { reschedule(job); }
    }
}


inline void free_slot(void * d) {
    xbt_free((*(slot_t *)d));
}


slot_t new_slot(int node_id, int it, double start_time, double duration) {
    slot_t s = NULL;
    
    s = xbt_malloc(sizeof(*s));
    s->node = node_id;
    s->start_time = start_time; 
    s->duration = duration;
    s->position = it;
    s->host = MSG_host_self();
    s->data = NULL;

    return s;
}


/* Compare 2 slots - needed for qsort */
int comp(const void * slot1, const void * slot2) {
    slot_t h1 = *((slot_t *) slot1), h2 = *((slot_t *) slot2);
    
    if (h1->start_time == h2->start_time) 
	return (h1->node < h2->node)? -1: 1;
    else 
	return (h1->start_time < h2->start_time)? -1: 1; 
} 


/* 
 * Slot * is an array of slots sorted by start_time. 
 * check_slot_validity check how many slot have the same start time. 
 */
int check_matching_slot(slot_t * slots, int nb_nodes)
{
    int i = 1;
    
    while (i < nb_nodes) { 
	if (slots[i-1]->start_time != slots[i]->start_time) 
	    return i;
	i++;
    }
    
    return i;
}


/* search a slot with param start_time and duration */
slot_t search_a_slot(cluster_t cluster, int node_id, int it, 
		     double start_time, double end_time, double duration) {
    xbt_dynar_t queue = cluster->waiting_queue[node_id];
    
    /* start_time is in a task -> we go to the tasks'end */
    if (start_time < end_time)
	return search_a_slot(cluster, node_id, it, end_time, 
			     end_time, duration);
    
    /* case : end of queue */ 
    if (it == xbt_dynar_length(queue)) {
#ifdef DEBUG
	printf ("[Node %d] Case : end of queue\n", node_id + 1);
#endif 
	return new_slot(node_id, it, start_time, -1.0);
    }
    else {
	/* we retrieve infos */
	job_t * job = NULL;
	
	job = (job_t *)xbt_dynar_get_ptr(queue, it);
	
	if (start_time < (*job)->start_time) {
	    if ((*job)->start_time - start_time > duration)
		return new_slot(node_id, it, start_time, 
				(*job)->start_time - start_time);
	    else
		/* we search next slot that match */
		return search_a_slot(cluster, node_id, it + 1, 
				     (*job)->start_time + (*job)->wall_time,
				     (*job)->start_time + (*job)->wall_time, 
				     duration);
	}
	else
	    /* recursive call */
	    return search_a_slot(cluster, node_id, it + 1, start_time, 
				 (*job)->start_time + (*job)->wall_time,
				 duration);
    }
}


/* Find  the first slot defined by a start_time, a duration and a nb_procs */
slot_t * find_a_slot(cluster_t cluster, int nb_nodes, double start_time,
		     double duration) {
    slot_t * slots = NULL;
    int i = 0 , match = 0;
    
    /* Init the tab of slots */
    slots = xbt_malloc(cluster->nb_nodes * sizeof(*slots));
    
    /* Init the tab by finding a first slot for each queue */
    for (i=0; i < cluster->nb_nodes; i++)
	slots[i] = search_a_slot(cluster, i, 0, start_time, 
				 start_time, duration);
    
    do { 
	for (i=0; i<match; i++) {	
	    slot_t temp = slots[i];
#ifdef DEBUG
	    printf("New slot search : %d; position : %d, start_time : %lf\n",
		    slots[i]->node+1, slots[i]->position, 
		    slots[match]->start_time);
#endif
	    /* 
	       slots[i] = search_a_slot(cluster, slots[i]->node, 0,
	       slots[match]->start_time, slots[match]->start_time, duration);
	    */
	    slots[i] = search_a_slot(cluster, slots[i]->node, 
				     slots[i]->position, 
				     slots[match]->start_time, 
				     slots[match]->start_time, duration);
	    xbt_free(temp);
	}
	
	/* Sort the result according to the slots start_time */
	qsort(slots, cluster->nb_nodes, sizeof(*slots), comp);
#ifdef DEBUG 
	print_slot(slots, cluster->nb_nodes);
#endif
	/* We check how many slots have the same start_time */
	match = check_matching_slot(slots, nb_nodes);
    }
    while(match != nb_nodes);
    
#ifdef DEBUG
    printf("slot is appropriate\n");
#endif
    /* We found the slot matching the request so we fill it in */
    return slots;
}


slot_t get_last_slot(cluster_t cluster, int i) {
    slot_t s;
    
    s = new_slot(i, xbt_dynar_length(cluster->waiting_queue[i]), 0, -1);
    
    if (xbt_dynar_length(cluster->waiting_queue[i]) != 0) {
	job_t  * last_job;
	
	last_job = (job_t *)xbt_dynar_get_ptr(cluster->waiting_queue[i], 
					      xbt_dynar_length
					      (cluster->waiting_queue[i]) 
					      -1);
	
	s->start_time = (*last_job)->start_time + (*last_job)->wall_time;
    }
    if (s->start_time < MSG_get_clock())
	s->start_time = MSG_get_clock();
    
    return s;
}


/* Could be speed up with a quick sort (depends of the average of nb )*/
slot_t * select_n_slots(cluster_t cluster, xbt_dynar_t slots, int nb) {
    int i;
    slot_t s; 
    slot_t * best_slots = NULL;
    
    best_slots = xbt_malloc(nb * sizeof(*best_slots));
    
    for (i=0; i<nb; i++) {
	int cursor = 0, node = 0;
	slot_t winning_slot = NULL;
	
	xbt_dynar_foreach(slots, cursor, s) {
		if (!winning_slot) winning_slot = s;
		if (winning_slot->start_time > s->start_time) {
		    winning_slot = s;
		    node = cursor;
		}
	}
	/* we retrieve the best bid of the market */
	xbt_dynar_remove_at(slots, node, &(best_slots[i]));
    }
    
    return best_slots;
}


double get_completion_time(cluster_t cluster) {
    int i;
    double start_time = 0.0;
    
    for (i = 0; i < cluster->nb_nodes; i++) {
	slot_t s = NULL;
	
	/* a speedup could be to not return a bid (no reusability) */
	s = get_last_slot(cluster, i);
	if (s->start_time > start_time)
	    start_time = s->start_time;
	xbt_free(s);
    }
    
    return start_time;
}


inline void print_slot(slot_t * slots, int size) {
    int i = 0;
    
    for (i=0 ; i<size; ++i) {
	printf("node : %d, idx: %d, start_time : %lf, duration : %lf, host: %s\n", 
               slots[i]->node+1, slots[i]->position, slots[i]->start_time,
               slots[i]->duration, slots[i]->host->name);
    }
}


inline void print_slots(xbt_dynar_t slots) {
    int cursor = 0;
    slot_t s;
    
    xbt_dynar_foreach(slots, cursor, s) {
	printf("slot %d : %f\t", s->node, s->start_time);
    }
    printf("\n");
}


/* Return the next job to schedule in the queue nb */
job_t next_job_to_schedule_in_queue(xbt_dynar_t queue) {
    int cpt;
    job_t job = NULL;

    /* No jobs in the queue */ 
    if (xbt_dynar_length(queue) == 0)
	return NULL;
    
    xbt_dynar_foreach(queue, cpt, job) {	 
	if (job->state == WAITING || job->state == RESERVED)
	    return job;
    }

    /* if there is just one job and it is scheduling */
    return NULL;
}


/* Return the next job to schedule */
job_t next_job_to_schedule(cluster_t cluster) {
    int i = 0;
    job_t res = NULL;
    
    for (i=0; i < cluster->nb_nodes; i++) {
	job_t job = next_job_to_schedule_in_queue(cluster->waiting_queue[i]);

	if ((res) && (job)) {
	    if (job->start_time < res->start_time)
		res = job;
	}
	else
	    res = (res)? res : job;
    }

    return res;
}
