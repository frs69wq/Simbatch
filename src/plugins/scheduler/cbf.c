/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

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

#include "plugin_scheduler.h"
#include "job.h"
#include "cluster.h"
#include "scheduling.h"


/************* Functions *************/
static void cbf_schedule(cluster_t cluster, job_t job);


/**************** Code ***************/
plugin_scheduler_t init (plugin_scheduler_t p)
{
    p->schedule = cbf_schedule;
    //p->reschedule = generic_no_reschedule;
    p->reschedule = generic_reschedule;
    //p->predict = cbf_predict;
    
    return p;
}


void cbf_schedule(cluster_t cluster, job_t job)
{
    int i = 0;
    slot_t  * slots;
    
#ifdef DEBUG
    fprintf(stdout, "\n- Scheduling %s -\n", job->name);
#endif
    job->run_time;
    slots = find_a_slot(cluster, job->nb_procs,
			MSG_get_clock(), job->wall_time);
    
    job->start_time = slots[0]->start_time;
    for (i=0; i<job->nb_procs; ++i)
    {
	job->mapping[i] = slots[i]->node; 
	xbt_dynar_insert_at(cluster->waiting_queue[slots[i]->node], 
			    slots[i]->position, &job);
    }
    
    xbt_free(slots);
}
