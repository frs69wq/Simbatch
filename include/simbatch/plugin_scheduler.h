#ifndef _PLUGIN_SCHEDULER_H_
#define _PLUGIN_SCHEDULER_H_

#include "cluster.h"
#include "job.h"

typedef struct _plugin_scheduler * plugin_scheduler_t;
typedef slot_t * (*scheduling_fct)(cluster_t, job_t);
typedef void (*reschedule_fct)(cluster_t cluster, plugin_scheduler_t scheduler);
typedef void (*accept_fct)(cluster_t cluster, job_t job, slot_t * slots);

/* API */
typedef struct _plugin_scheduler {
    char * name;
    scheduling_fct schedule;
    reschedule_fct reschedule;
    accept_fct accept;
} plugin_scheduler;


/* Loading function */
plugin_scheduler_t init(plugin_scheduler_t p);


#endif
