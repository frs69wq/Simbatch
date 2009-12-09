/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include <msg/msg.h>

#include "simbatch.h"
#include "algos.h"


/*
 * Returns the best choice (according to MCT) to execute a task
 */
p_winner_t MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job) {
  int i = 0;
  double completionT = DBL_MAX;
  p_winner_t winner = xbt_malloc(sizeof(winner_t));
  int failure = 0;
  char name[] = "metasched_MB";
  char sed_MB[256];
  
  winner->completionT = DBL_MAX;
  winner->job = job;
    
   
  for (i = 0; i < nbClusters; ++i) {
    m_task_t task = NULL;
    
    sprintf(sed_MB, "client-%s", clusters[i]->name);
    /* Ask when the cluster will be able to execute the job */
    MSG_task_send(MSG_task_create("SED_PRED", 0, 0, job), 
		 sed_MB);
    MSG_task_receive(&task, name);
    if (!strcmp(task->name, "SB_PRED")) { // OK
      slot_t * slots = NULL;
      slots = MSG_task_get_data(task);
      completionT = slots[0]->start_time + (job->wall_time * speedCoef[i]);
      if (completionT < winner->completionT) { 
	winner->completionT = completionT;
	winner->cluster = slots[0]->host;
      }
      xbt_free(slots);
    }
    else { 
      const m_host_t sender = MSG_task_get_source(task);
      if (!strcmp(task->name, "SB_SERVICE_KO"))
	printf("Service unavailable on host: %s\n", sender->name);
      else
	printf("Unadequate resources: %s\n", sender->name);

      ++failure;
    }
    MSG_task_destroy(task), task = NULL;
  }

  /* No adequate resource to execute a job */
  if (failure == nbClusters) { 
    winner->completionT = -1;
  }

  return winner;
}
