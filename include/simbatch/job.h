/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/


#ifndef _JOB_H_
#define _JOB_H_

#include <msg/msg.h>

typedef enum _state_t {
    WAITING    = 0,
    LOADING    = 1,
    PROCESSING = 2,
    RECOVERING = 3,
    DONE       = 4,
    CANCELLED  = 5,
    RESERVED   = 6
} state_t;


/*
 * Let's define a job : 
 * - submit time : time client send the job
 * - entry time : time the job entered for the first time in the batch
 * - run time : time job runs
 * - weight : a weight given by some heuristic
 * - data size 
 * - requested time
 * - #procs needed
 * - mapping : proc # affected for the task
 * - a state 
 * - job id fixed by the batch
 * - source is the m_host_t which send the task
 */

typedef struct _job {
    char name[15];
    char service[15];
    double submit_time;
    double entry_time;
    double run_time;
    double input_size;
    double output_size;
    double wall_time;
    double start_time;
    double weight;
    unsigned long int id;
    int priority;
    int nb_procs;
    int * mapping;
    state_t state;
    m_host_t source;
    void * data;
} * job_t;


/* 
 * Slot (or non-job) definition : 
 * node : node number which owns the slot
 * position : slot position in the waiting queue
 * start_time : start time of the slot
 * duration : slot duration
 * host : usefull for a metascheduler
 * data : could be usefull
 */
typedef struct _slot {
    int node;
    int position;
    double start_time;
    double duration;
    m_host_t host;
    void * data;
} slot, * slot_t;


#endif
