/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#ifndef _PLUGIN_SCHEDULER_H_
#define _PLUGIN_SCHEDULER_H_

#include <msg/msg.h>
#include "cluster.h"
#include "scheduling.h"

/* API */
typedef struct {
    char * name;
    scheduling_fct schedule;
    /* any function of scheduling */
    reschedule_fct reschedule;
    prediction_fct predict;
} plugin_scheduler, * plugin_scheduler_t;


/* Loading function */
plugin_scheduler_t init(plugin_scheduler_t p);


#endif
