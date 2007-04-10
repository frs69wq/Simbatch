/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/


#ifndef _JOB_H_
#define _JOB_H_


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
 * - data size 
 * - requested time
 * - #procs needed
 * - mapping : proc # affected for the task
 * - a state 
 * - job id fixed by the batch 
 */


typedef struct _job {
    char name[15];
    double submit_time;
    double entry_time;
    double run_time;
    double input_size;
    double output_size;
    double wall_time;
    double start_time;
    unsigned long int id;
    int priority;
    int nb_procs;
    int * mapping;
    state_t state; /* to avoid data duplication */
} * job_t;

#endif
