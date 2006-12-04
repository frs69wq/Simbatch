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

/************* Functions *************/
static void fcfs_schedule(cluster_t cluster, job_t job);

static double fcfs_predict(cluster_t cluster, job_t job);

/**************** Code ***************/
plugin_scheduler_t init (plugin_scheduler_t p)
{
    p->schedule = fcfs_schedule;
    //p->reschedule = generic_no_reschedule;
    p->reschedule = generic_reschedule;
    //p->predict = fcfs_predict();
  
    return p;
}

static void fcfs_schedule(cluster_t cluster, job_t job)
{
    int i = 0;
  slot_t * best_slots = NULL;
  xbt_dynar_t slots = NULL;

  slots = xbt_dynar_new(sizeof(slot_t), free_slot);
 
  /* We take the slot for each queue */
  for (i=0; i<cluster->nb_nodes; i++)
    {
      slot_t s;
      s = get_last_slot(cluster,i);
      xbt_dynar_push(slots, &s);
    }
    
  /* We select the n best bids (the lowest date) */
  best_slots = select_n_slots(cluster, slots, job->nb_procs);

  /* The start_time of the job is finally equal to the
     worst start_time among the best */
  job->start_time = best_slots[job->nb_procs-1]->start_time;

  /* we must push the task in its waiting queue(s) */
  for (i=0; i<job->nb_procs; i++)
    {
      job->mapping[i] = best_slots[i]->node;
      xbt_dynar_push(cluster->waiting_queue[best_slots[i]->node], &job);
    }
  
  /* cleaning every strtuctures */
  xbt_dynar_free(&slots);

  for (i=0;i<job->nb_procs;i++)
    xbt_free(best_slots[i]);
  xbt_free(best_slots);
}
 
/* Selection of the n best slots (n = nb_procs needed) */


static double fcfs_predict(cluster_t cluster, job_t job)
{
  int i = 0;
  slot_t * best_slots = NULL;
  xbt_dynar_t slots = NULL;
  double prediction = 0;

  slots = xbt_dynar_new(sizeof(slot_t), free_slot);
 
  /* We take the bid for each queue */
  for (i=0; i<cluster->nb_nodes; i++)
    {
      slot_t b;
      b = get_last_slot(cluster, i);
      xbt_dynar_push(slots, &b);
    }
    
  /* We select the n best slots (the lowest date) */
  best_slots = select_n_slots(cluster, slots, job->nb_procs);  

  /* The start_time of the job is finally equal to the 
     worst start_time among the best */
  prediction = best_slots[job->nb_procs-1]->start_time;

  /* cleaning every strtuctures */
  xbt_dynar_free(&slots);
  for (i=0; i<job->nb_procs; i++)
    xbt_free(best_slots[i]);
  xbt_free(best_slots);

  return prediction;
}
