/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


/**
 * \file job.h
 * Define the job_t, slot_t and state_t datatypes .
 */


#ifndef _JOB_H_
#define _JOB_H_

#include <msg/msg.h>

/*
 * States a job can have
 */
typedef enum state_t {
    WAITING    = 0,
    LOADING    = 1,
    PROCESSING = 2,
    RECOVERING = 3,
    DONE       = 4,
    CANCELLED  = 5,
    RESERVED   = 6
} state_t;


/*
 * Job Definition:
 * - name: Name of the job 
 * - submit_time: time client send the job
 * - entry_time: time the job entered for the first time in the batch
 * - run_time: time needed to run
 * - input_size: Size to be transferred before execution
 * - output_size: size of produced data
 * - wall_time: Duration for a slot requested by the job
 * - start_time: time teh job started execution
 * - weight: a weight given by some heuristic (used by HPF)
 * - user_id: id given by the user
 * - id: id given by the batch
 * - priority: priority of a job
 * - nb_proc: Number of procs needed
 * - service: Service that can execute the job
 * - mapping: proc affected for the job
 * - state: current state of a job
 * - source: the m_host_t which send the task
 * - free_on_completion: says if the job should be freed when completed
 * - data: some datas
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
    double completion_time;
    double weight;
    unsigned long int user_id;
    unsigned long int id;
    int priority;
    int nb_procs;
    unsigned int service;
    int * mapping;
    state_t state;
    m_host_t source;
    int free_on_completion;
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
