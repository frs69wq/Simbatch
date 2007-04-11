/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#include <stdio.h>
#include <stdlib.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <msg/msg.h>

#include "plugin_scheduler.h"
#include "job.h"
#include "cluster.h"
#include "scheduling.h"

///// DEPRECATED

/************* Functions *************/
static slot_t * rrobin_schedule(cluster_t cluster, job_t job);

/**************** Code ***************/
plugin_scheduler_t init (plugin_scheduler_t p) {
    p->schedule = rrobin_schedule;
    p->reschedule = generic_reschedule;
    
    return p;
}


static slot_t * rrobin_schedule(cluster_t cluster, job_t job) {
    static int proc = 0;
    int j = 0, k = proc;
    job->start_time = 0.0;
    
    /* We take the bid for each queue */
    for (j = 0; j < job->nb_procs; ++j) {
	slot_t b = NULL;
	
	/* a speedup could be to not return a bid (no reusability) */
	b = get_last_slot(cluster, k);
	if (b->start_time > job->start_time)
	    job->start_time = b->start_time;
	k = (k+1) % cluster->nb_nodes;
	xbt_free(b);
    }
    
    for (j=0; j<job->nb_procs; ++j) {
	job->mapping[j] = proc;
	xbt_dynar_push(cluster->waiting_queue[proc], &job);
	proc = (proc+1) % cluster->nb_nodes;
    }

    return NULL;
}

static void rrobin_accept(cluster_t cluster, job_t job, slot_t * slots) {
    return;
}

