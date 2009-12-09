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
 * Define the job, slot and state datatypes.
 * \todo better coding style:
 * typedef job_t {...} job_t;
 * typedef job_t *m_job_t; etc... 
 */


#ifndef _JOB_H_
#define _JOB_H_

#include <msg/msg.h>

/**
 * States a job can have.
 */
typedef enum state_t {
  WAITING    = 0,
  LOADING    = 1,
  PROCESSING = 2,
  RECOVERING = 3,
  DONE       = 4,
  CANCELLED  = 5,
  RESERVED   = 6,
  ERROR      = 7
} state_t;


/**
 * Job structure.
 * Simbatch defines a job strutures to add informations to simbatch m_task_t
 * types.
 */
typedef struct _job {
  char name[15];      /*!< Name of the job. */
  double submit_time; /*!< Time client send the job. */
  double entry_time;  /*!< First time the job enters the batch. */
  double run_time;    /*!< Time needed to run. */
  double input_size;  /*!< Size to be transferred before execution. */
  double output_size; /*!< Size of produced data. */
  double wall_time;   /*!< Duration for a slot requested by the job. */
  double start_time;  /*!< Time when job starts its execution. */
  double completion_time;     /*!< Time when job completes its execution. */
  double weight;              /*!< A weight given by some heuristic. */
  double deadline;    /*!< Deadline to complete the job */
  unsigned long int user_id;  /*!< Jobs'id given by the user. */
  unsigned long int id;   /*!< Jobs'id given by the batch */
  int priority;       /*<! priority of a job. */
  unsigned int nb_procs;       /*<! Number of procs needed. */
  unsigned int service;   /*<! Service that can execute the job. */
  int * mapping;      /*<! Proc affected for the job. */
  state_t state;      /*<! Current state of a job. */
  m_host_t source;    /*<! The m_host_t which send the task. */
  int free_on_completion; /*<! Job to free when completed */
  void * data;        /*<! some datas */
} * job_t;


/**
 * Slot definition.
 * In the Gantt diagram, it exists two kind of slots: empty and full slots.
 * They define the time available or unavailable on a cluster.
 * Only full slots are technically existing in Simbatch.
 */
typedef struct _slot {
  unsigned int node;     /*<! Number designing the job owning the slot. */
  unsigned int position; /*<! Position in the waiting queue. */
  double start_time;     /*<! Start time of the slot.*/
  double duration;       /*<! Slot duration. */
  m_host_t host;         /*<! Usefull for a metascheduler. */
  void * data;           /*<! Could be usefull. */
} slot, * slot_t;


#endif
