/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _PLUGIN_SCHEDULER_H_
#define _PLUGIN_SCHEDULER_H_

#include "cluster.h"
#include "job.h"

typedef struct plugin_scheduler *plugin_scheduler_t;
typedef slot_t * (*scheduling_fct)(m_cluster_t, job_t);
typedef void (*reschedule_fct)(m_cluster_t cluster, plugin_scheduler_t scheduler);
typedef void (*accept_fct)(m_cluster_t cluster, job_t job, slot_t * slots);

/* API */
typedef struct plugin_scheduler {
    char *name;
    scheduling_fct schedule;
    reschedule_fct reschedule;
    accept_fct accept;
} plugin_scheduler;


/* Loading function */
plugin_scheduler_t
init(plugin_scheduler_t p);


#endif
