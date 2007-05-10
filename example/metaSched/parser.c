/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#include <stdlib.h>
#include <stdio.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include "simbatch.h"
#include "parser.h"


/*
 * fields :
 * 1 -> submit_time : job will be submit at time t
 * 2 -> run_time : time needed for the job to be done
 *      (it will be soon amount of computation)
 * 3 -> input_size : amount of data to transfert before treatment
 * 4 -> ouput_size : amount of data to transfert after
 * 5 -> requested time : define the end size of a reservation
 * 6 -> nb_procs : nb procs requested for a task   
 * 7 -> priority : priority of the task
 */

xbt_fifo_t parse(const char * wld_file, const char * name) {
    int i = 1;
    FILE * f = NULL;
    char buf[512];
    xbt_fifo_t list = NULL;
    
    
    /* Papa's got a brand new bag... */
    printf ("parsing %s\n", wld_file);
    if ((f = fopen(wld_file, "r")) == NULL) {
	fprintf (stderr, "%s:line %d, function %s, fopen failed : %s \n",\
		 __FILE__, __LINE__, __func__, wld_file);
	return NULL;
    }
    
    list = xbt_fifo_new();
    
    while (fgets(buf,512,f) != NULL) {
	/* Comments */
	if (buf[0] != ';') {
	    job_t job;
	    
	    job = xbt_malloc(sizeof(*job));
	    job->start_time = 0.0;
	    /* Need to ajsust and keep only the usefull data */
	    sscanf(buf,"%lf %lf %lf %lf %lf %d %d %s", &(job->submit_time),\
		   &(job->run_time), &(job->input_size), &(job->output_size),\
		   &(job->wall_time), &(job->nb_procs), &(job->priority), job->service);
	    job->state = WAITING;
            job->weight = 0.0;
	    sprintf(job->name, "%s%d", name, i++);
	    xbt_fifo_push(list,job);
	    printf("%s %lf %lf %lf %lf %d %s\n", job->name, job->submit_time,\
		   job->run_time, job->input_size, job->wall_time, job->nb_procs,
                   job->service);
	}
    }
    fclose(f);
    
    return list;
}
