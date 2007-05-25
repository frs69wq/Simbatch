/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


/*********  Very Simple Parser **************
* Parse file from the swf format
* (Feitelson's format).
* 
* DEPRECATED 
*********************************************/  

#include <stdlib.h>
#include <stdio.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include "plugin_input.h"
#include "job.h"


/******** Functions ********/
static xbt_fifo_t swf_parse(const char * swf_file, const char * name);

/***************************/

plugin_input_t init_input(plugin_input_t p)
{
  p->create_list = swf_parse;

  return p;
}

/* 
 * Just parse a swf file for the moment and 
 * return a xbt_list of corresponding jobs 
 *
 * awk '{print($2,$4,$8,$9)}' file.swf
 * 
 * $2 : time at wich the job will be executed
 * $4 : time during job will run
 * $8 : nb_procs 
 * $9 : time for the deadline 
 * 
 */
static xbt_fifo_t swf_parse(const char * swf_file, const char * name)
{
  int i = 1;
  FILE * f = NULL;
  char buf[512];
  xbt_fifo_t list = NULL;

  
  /* Papa's got a brand new bag... */
  if ((f = fopen(swf_file, "r")) == NULL)
    {
      fprintf (stderr,"%s:%d,fopen\n",__FILE__,__LINE__);
      return NULL;
    }

  list = xbt_fifo_new();
  
  while (fgets(buf,512,f) != NULL)
    {
      /* Comments */
      if (buf[0] != ';')
	{
	  job_t job;
		
	  job = xbt_malloc(sizeof(*job));
	  job->start_time = 0.0;
	  /* Need to ajsust and keep only the usefull data */
	  sscanf(buf,"%*d %lf %*d %lf %*d %*d %*d %d %lf %*d %*d %*d %*d %*d %*d %*d %*d %*d",\
		 &(job->submit_time),&(job->run_time),&(job->nb_procs),&(job->wall_time));
	  job->nb_procs /= 8;
	  job->input_size = 0.0;
	  job->output_size = job->input_size;
	  job->state = WAITING;
	  job->priority = 0;
	  sprintf(job->name,"%s%d", name, i++);
	  xbt_fifo_push(list,job);
#ifdef DEBUG
	  printf("%s %lf %lf %d %lf\n", job->name, job->submit_time,\
		 job->run_time, job->nb_procs, job->wall_time);
#endif
	}
    }
  fclose(f);

  return list;
}
