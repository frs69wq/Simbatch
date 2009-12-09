/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <xbt/dict.h>
#include <msg/msg.h>
#include <xbt/ex.h>

#include "simbatch_config.h"
#include "utils.h"
#include "ports.h"
#include "job.h"
#include "node.h"


int
SB_node(int argc, char **argv)
{ 
  m_task_t task = NULL;
  char name[256];
  MSG_error_t err = MSG_OK;
  
  sprintf(name, "Node-%s", HOST_NAME());

#ifdef LOG
  FILE * flog;
#endif

#ifdef VERBOSE
  fprintf(stderr, "%s... ready\n", HOST_NAME()); 
#endif
  while (1) {
    /* Waiting for input comm */
    err = MSG_task_receive_with_timeout(&task, name, DBL_MAX);
    if (err == MSG_TIMEOUT_FAILURE) {
      break;
    }
    
#ifdef LOG
    flog = config_get_log_file(
	      MSG_host_get_name(MSG_task_get_source(task))); 
    if (strcmp(task->name, "DATA_IN") == 0)
      fprintf(flog, "[%lf]\t%20s\tData Received from %s\n", 
	      MSG_get_clock(), PROCESS_NAME(), 
	      MSG_process_get_name(MSG_task_get_sender(task)));
#endif
    MSG_task_destroy(task);
    task = NULL;
  }
  return EXIT_SUCCESS;
}
