#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "myutils.h"
#include <xbt.h>
#include "defs.h"

static xbt_fifo_t parse_SG_job(const char * file_name, const char * name);

int client (int argc, char ** argv) {

  xbt_fifo_t job_list;
  double time;
  SG_job_t job;
  m_host_t meta_scheduler;
  m_task_t task;
  winner_t * winner;

  if (argc < 3) {
    printf("Client badly defined in deployment file\n");
    exit(EXIT_FAILURE);
  }

  printArgs(argc, argv);
  
  //parse input file
  job_list = parse_SG_job(argv[1], "job");
  
  meta_scheduler = MSG_get_host_by_name(argv[2]);

  time = 0;
  while (xbt_fifo_size(job_list) > 0) {
    job = xbt_fifo_shift(job_list);

    MSG_process_sleep(job->submit_time - time);
    MSG_task_put(MSG_task_create("SB_ASK_SCHED", 0, 0, job), 
                 meta_scheduler, MS_CHAN);
    task = NULL;
    MSG_task_get(&task, CLIENT_CHAN);
    winner = (winner_t*)MSG_task_get_data(task);
    MSG_task_destroy(task);
    time = job->submit_time;
    if (winner != NULL) {
#ifdef VERBOSE
      printf("[%lf]Client sends %s on %s (%d procs) (CT: %lf, runT: %lf) \n", 
             time, winner->job->name, winner->cluster->name, 
             winner->job->nb_procs, winner->completionT, 
             winner->job->run_time);//*/
#endif
      MSG_task_put(MSG_task_create("SB_TASK", 0, 
                                   winner->job->input_size, winner->job),
                   winner->cluster, SED_CHAN);
    }
    else {
#ifdef VERBOSE
      printf("metaShced found no adequate batch to execute %s, "
	     "job is canceled\n", 
             job->name);
#endif
      xbt_free(job);
    }

#ifdef VERBOSE
    printf("\n");
#endif

  }
#ifdef VERBOSE  
  printf("All jobs sended\n\n");
#endif

  xbt_fifo_free(job_list);
  return EXIT_SUCCESS;
}


static xbt_fifo_t parse_SG_job(const char * file_name, const char * name) {
  FILE * f = NULL;
  char buf[512];
  xbt_fifo_t list = NULL;
#ifdef VERBOSE  
  printf ("Parsing %s...", file_name);
#endif
  f = fopen(file_name, "r");
  
  if (f == NULL) {
    fprintf (stderr, "%s:line %d, function %s, fopen failed : %s \n",   \
             __FILE__, __LINE__, __func__, file_name);
    return NULL;
  }
  
  list = xbt_fifo_new();

  SG_job_t job;
  while (fgets(buf,512,f) != NULL) {
    /* Comments */
    if (buf[0] != ';' && buf[0] != '#') {
      job = xbt_malloc(sizeof(*job));
      /* Need to ajsust and keep only the usefull data */
      sscanf(buf,"%lu %lf %lf %lf %d %d %u %lf", 
             &(job->user_id), &(job->submit_time), 
             &(job->input_size), 
             &(job->output_size), &(job->nb_procs), 
             &(job->service), &(job->priority), &(job->wall_time));
      sprintf(job->name, "%s%lu", name, job->user_id);
      xbt_fifo_push(list,job);
    }
  }
  fclose(f);
#ifdef VERBOSE
  printf ("OK\n");
#endif
  return list;
}
