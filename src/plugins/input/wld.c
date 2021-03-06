/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2009 Simbatch Tean. All rights reserved.                   */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include "plugin_input.h"
#include "job.h"


/******** Functions ********/
static xbt_fifo_t wld_parse(const char * wld_file, const char * name);

/***************************/

plugin_input_t init_input(plugin_input_t p) {
  p->create_list = wld_parse;  
  return p;
}

/*
 * fields :
 * 1 -> submit_time : job will be submit at time t
 * 2 -> run_time : time needed for the job to be done
 *      (it will be soon amount of computation)
 * 3 -> input_size : amount of data to transfert before treatment
 * 4 -> ouput_size : amount of data to transfert after
 * 5 -> requested time : define the end size of a reservation
 * 6 -> nb_procs : nb procs requested for a task   
 * 7 -> service : type of job
 * 8 -> priority : priority of the task
 */
static xbt_fifo_t wld_parse(const char * wld_file, const char * name) {
  int i = 1;
  FILE * f = NULL;
  char buf[512];
  xbt_fifo_t list = NULL;
  job_t job;
  
#ifdef VERBOSE
  printf ("Parsing %s...", wld_file);
#endif
  
  /* Papa's got a brand new bag... */
  if ((f = fopen(wld_file, "r")) == NULL) {
    fprintf (stderr, "%s:line %d, function %s, fopen failed : %s \n",\
	     __FILE__, __LINE__, __func__, wld_file);
#ifdef VERBOSE
    printf ("failed\n");
#endif
    return NULL;
  }
  
  list = xbt_fifo_new();
  
  while (fgets(buf,512,f) != NULL) {
    /* Comments or empty lines*/
    if (buf[0] != ';' && buf[0] != '\n') {
      job = xbt_malloc(sizeof(*job));
      job->start_time = 0.0;
      sscanf(buf,"%lu %lf %lf %lf %lf %lf %d %d %u", 
	     &(job->user_id), &(job->submit_time), &(job->run_time),
	     &(job->input_size), &(job->output_size), &(job->wall_time),
	     &(job->nb_procs), &(job->priority), &(job->service));
      job->state = WAITING;
      job->free_on_completion = 1;
      job->mapping = NULL;
      job->deadline = -1;
      sprintf(job->name, "%s%lu", name, job->user_id);
      xbt_fifo_push(list,job);
#ifdef DEBUG
      printf("%s %lf %lf %lf %lf %d %u\n", job->name, job->submit_time,
	     job->run_time, job->input_size, job->wall_time, 
	     job->nb_procs, job->service);
#endif
    }
  }
  fclose(f);
  
#ifdef VERBOSE
  printf ("done\n");
#endif
  
  return list;
}
