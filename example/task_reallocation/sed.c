#include "sed.h"
#include "defs.h"
#include <xbt.h>
#include "myutils.h"
#include <float.h>
#include <stdio.h>

static winner_t * get_job(m_host_t host, m_host_t batch, SG_job_t SGjob);
static void update_winners(xbt_dynar_t winners, xbt_dynar_t to_free);
static void remove_job(xbt_dynar_t list, job_t job);

/* result_size = 0 if the speedup function is known
 * result_size = -1 if the service is not found
 * result_size = length of results
 */
static perf * parse_perf_file(const char * file_name, const int service, 
                              int * result_size);

int sed(int argc, char ** argv) {
  m_task_t task;
  m_host_t batch;
  m_host_t metaSched;
  MSG_error_t err;
  xbt_fifo_t msg_stack;
  xbt_dynar_t winners;
  xbt_dynar_t to_free;
  winner_t * w;

  printArgs(argc, argv);

  batch = MSG_get_host_by_name(argv[2]);
  metaSched = MSG_get_host_by_name(argv[1]);

  //sed receives tasks
  msg_stack = xbt_fifo_new();
  winners = xbt_dynar_new(sizeof(job_t*), NULL);
  to_free = xbt_dynar_new(sizeof(job_t*), NULL);

  while (1) {

    task = NULL;
    err = MSG_task_get_with_time_out(&task, SED_CHAN, DBL_MAX);
    if (err == MSG_TRANSFER_FAILURE) { 
      break; 
    }
    if (err == MSG_OK) {
      xbt_fifo_push(msg_stack, task);
      while (MSG_task_Iprobe(SED_CHAN)) {
	task = NULL;
	MSG_task_get(&task, SED_CHAN);
	xbt_fifo_push(msg_stack, task);
      }
    }
   
    while (xbt_fifo_size(msg_stack) > 0) {

      task = xbt_fifo_shift(msg_stack);

      if (strcmp(task->name, "SB_ASK_SCHED") == 0) {
        w = get_job(MSG_host_self(), batch, 
                    (SG_job_t)MSG_task_get_data(task));
        if (w == NULL ){
          printf("ERROR: not supposed to be here\n");
        }
        else {
          MSG_task_put(MSG_task_create("SB_RESP_SCHED", 0, 0, w), 
                       metaSched, MS_CHAN);
        }
      }//SB_ASK_SCHED

      else if (strcmp(task->name, "SB_TASK") == 0) {
        job_t job = MSG_task_get_data(task);
        xbt_dynar_push(winners, &job);
        //printf("%s %s %lf\n", job->name, ((SG_job_t)job->data)->name, job->completion_time);
        MSG_task_put(MSG_task_create("SB_TASK", job->input_size, 0, job),
                                     batch, CLIENT_PORT);
      }//SB_TASK

      else if (strcmp(task->name, "SB_TASK_CANCEL") == 0) {
        job_t job;
        job = ((cancelation*)MSG_task_get_data(task))->job;
        MSG_task_put(MSG_task_create("SB_TASK_CANCEL", 0, 0, job), 
                     batch, CLIENT_PORT);
        remove_job(winners, job);
        //MSG_task_put(MSG_task_create("TEST", 0, 0, NULL), metaSched, MS_CHAN);
        printf("[%lf]%s canceled on batch %s\n", MSG_get_clock(), 
               job->name, 
               MSG_host_get_name(MSG_host_self()));
      }//SB_TASK_CANCEL

      else if (strcmp(task->name, "SB_TRANSMIT_TASK") == 0) {
        job_t job;
        job = ((cancelation*)MSG_task_get_data(task))->job;
        m_host_t host;
        host = ((cancelation*)MSG_task_get_data(task))->host;
        if (host != NULL) {
          remove_job(winners, job);
          MSG_task_put(MSG_task_create("SB_TASK", job->input_size, 0, job),
                       host, SED_CHAN);
          printf("[%lf]transmitted %s from %s to %s\n", 
                 MSG_get_clock(), job->name,
                 MSG_host_get_name(MSG_host_self()), 
                 MSG_host_get_name(host));
        }
        else {
          printf("%s resoumis au batch\n", job->name);
          MSG_task_put(MSG_task_create("SB_TASK", job->input_size, 0, job),
                                     batch, CLIENT_PORT);
        }
      }//SB_TRANSMIT_TASK

      else if (strcmp(task->name, "GET_JOBS") == 0) {
        update_winners(winners, to_free);
        xbt_dynar_t wins = xbt_dynar_new(sizeof(cancelation**), NULL);
        unsigned int i;
        cancelation ** can;
        for (i = 0; i < xbt_dynar_length(winners); i++) {
          can = (cancelation**)malloc(sizeof(cancelation*));
          *can = (cancelation*)malloc(sizeof(cancelation));
          (*can)->job = *((job_t*)xbt_dynar_get_ptr(winners, i));
          (*can)->host = MSG_host_self();
	  xbt_dynar_push(wins, can);
          can = NULL;
        }
        MSG_task_put(MSG_task_create("GET_JOBS", 0, 0, wins),
                     MSG_task_get_source(task), MS_JOB_CHAN);//*/
      }//GET_JOBS

      else {
        printf("Task unrecognized. Destroying it\n");
      }//Not supposed to be there
      MSG_task_destroy(task);
      task = NULL;
    }
  }
  
  printf("Sed on %s exited main loop\n", 
         MSG_host_get_name(MSG_host_self()));

  xbt_fifo_free(msg_stack);
  xbt_dynar_free(&winners);
  xbt_dynar_free(&to_free);
  
  return EXIT_SUCCESS;
}


static perf * parse_perf_file(const char * file_name, const int service, 
                       int * result_size) {
  FILE * f = NULL;
  char buf[512];
  char type[10] = "";
  int nb_type;
  int i;
  int know_speedup;
  int ref_power;
  perf * results;
  
  //printf ("Parsing %s...", file_name);
  f = fopen(file_name, "r");
  
  if (f == NULL) {
    fprintf (stderr, "%s:line %d, function %s, fopen failed : %s \n",   \
             __FILE__, __LINE__, __func__, file_name);
    return NULL;
  }


  while (fgets(buf,512,f) != NULL/* ||(atoi(type) != service)*/) {
    if (buf[0] != ';' && buf[0] != '#') {
      sscanf(buf, "%s %d", type, &nb_type);
      //if we found a wrong service, we go to the next
      if (atoi(type) != service) {
        for (i = 0; i <= nb_type; i++) {
          if (fgets(buf,512,f) == NULL) {
            exit(EXIT_FAILURE);
          }
        } 
      }
      //we found the right service
      else {
        if (fgets(buf, 512, f) == NULL) {
          exit(EXIT_FAILURE);
        }
        sscanf(buf, "%d %d", &know_speedup, &ref_power);
        if (know_speedup) {
          //Do something, but well see that later
          fclose(f);
          printf("OK\n");
          *result_size = 0;
          return NULL;
        }
        else { 
          results = xbt_malloc(nb_type * sizeof(perf));
          for (i = 0; i < nb_type; i++) {
            if (fgets(buf, 512, f) == NULL) {
              exit(EXIT_FAILURE);
            }
            results[i].reference_power = ref_power;
            sscanf(buf, "%d %lf", 
                   &(results[i].nb_proc), &(results[i].time));
          }
          fclose(f);
          //printf("OK\n");
          *result_size = nb_type;
          return results;
        }
      }
    }
  }
  fclose(f);
  //printf("OK\n");
  *result_size = -1;
  return NULL;
}



/*
 *Called by the SeD to choose on how many processors to execute a task
 */
static winner_t * get_job(m_host_t host, m_host_t batch, 
                               SG_job_t SGjob) {
  perf * perfs;
  int res_size;
  m_task_t task;
  int i;
  job_t job;
  winner_t * winner;
  double bestRT = 0.0;
  double bestWT = 0.0;
  double bestET = 0.0;
  int bestProc = 0;
  int * bestMapping = NULL;
  int failures;
  winner = xbt_malloc(sizeof(winner_t));
  job = xbt_malloc(sizeof(*job));  
  
  winner->completionT = DBL_MAX;

  strcpy(job->name, SGjob->name);
  job->user_id = SGjob->user_id;
  job->id = 0;
  job->submit_time = SGjob->submit_time;
  job->input_size = SGjob->input_size;
  job->output_size = SGjob->output_size;
  job->nb_procs = SGjob->nb_procs;
  job->service = SGjob->service;
  job->priority = SGjob->priority;
  job->state = WAITING;
  job->entry_time = 0.0;
  job->start_time = 0.0;
  job->weight = 0.0;
  job->free_on_completion = 0;

  perfs = parse_perf_file("perfs.txt", SGjob->service, &res_size);

  if (job->nb_procs < 0) {
    printf("Number of processors for job %s is < 0 : ERROR\n", SGjob->name);
    return NULL;
  }

  //if the user specified the number of processors
  //modify the perfs array with just one perf
  if (job->nb_procs > 0) {
    int found = 0;
    i = 0;
    while (i < res_size && !found) {
      if (perfs[i].nb_proc == job->nb_procs) {
        int a;
        unsigned long int b;
        double c;
        res_size = 1;
        a = perfs[i].nb_proc;
        b = perfs[i].reference_power;
        c = perfs[i].time;
        xbt_free(perfs);
        perfs = xbt_malloc(sizeof(perf));
        perfs[0].nb_proc = a;
        perfs[0].reference_power = b;
        perfs[0].time = c;
        found = 1;
      }
      i++;
    }
    
    if (!found) {
      res_size = -1;
    }
  }

  //the speedup function if known
  if (res_size == 0) {
    //do something (possibly like when we fix the number of processors)
    return NULL;
  }
  
  //if the service is not found in the description file
  if (res_size == -1) {
    //create the winner with a CT of -1
    printf("Not supposed to be here\n");
    return NULL;
  }

  //in the other cases, try every possibility of number of processors.
  //get the completion time for each number of processors
  failures = 0;
  for (i = 0; i < res_size; i++) {
    job->run_time = (perfs[i].reference_power / MSG_get_host_speed(host)) 
      * perfs[i].time;
    //we need to define a way to compute the wall_time
    job->wall_time = job->run_time * 2;// ((rand() % 10) + 2) ;
    job->nb_procs = perfs[i].nb_proc;
    MSG_task_put(MSG_task_create("SED_PRED", 0, 0, job), 
                 batch, CLIENT_PORT);
    task = NULL;
    MSG_task_get(&task, SED_CHANNEL);
    if (strcmp(task->name, "SB_PRED") == 0) { // OK
      slot_t * slots = NULL;
      double completionT;
      slots = MSG_task_get_data(task);
      completionT = slots[0]->start_time + job->wall_time;
      if (completionT < winner->completionT) { 
        int j;
        if (i != 0) {
          xbt_free(bestMapping);
        }
        bestMapping = xbt_malloc(job->nb_procs * sizeof(int));
        for (j = 0; j < job->nb_procs; j++) {
          bestMapping[j] = job->mapping[j];
        }
        xbt_free(job->mapping);
        bestET = job->entry_time;
        bestRT = job->run_time;
        bestWT = job->wall_time;
        bestProc = perfs[i].nb_proc;
        winner->completionT = completionT;
        winner->cluster = slots[0]->host;
      }
      xbt_free(slots);
    }
    else {
      failures++;
    }
    MSG_task_destroy(task);
  }
  job->entry_time = bestET;
  job->mapping = bestMapping;
  job->run_time = bestRT;
  job->wall_time = bestWT;
  job->nb_procs = bestProc;
  job->data = SGjob;
  winner->job = job;
  if (failures == res_size) {
    /*printf("Sed %s unable to execute job %s\n", 
      MSG_host_get_name(MSG_host_self()), job->name);//*/
    winner->completionT = -1;
  }
  //  printf("%s on %s : %lf\n", winner->job->name, winner->cluster->name, winner->completionT);
  xbt_free(perfs);  
  return winner;
}


void update_winners(xbt_dynar_t winners, xbt_dynar_t to_free) {
  unsigned int i;
  job_t * w;
  //tests if the jobs that have been submitted are still waiting
  for (i = 0; i < xbt_dynar_length(winners); i++) {
    w = NULL;
    w = xbt_dynar_get_ptr(winners, i);
    if ((*w)->state != WAITING) {
      xbt_dynar_push(to_free, w);
      //printf("[%lf]%s passe de winners a to_free\n", MSG_get_clock(), (*w)->name);
      w = NULL;
      xbt_dynar_remove_at(winners, i, w);
      i--;
    }
  }
  
  //free jobs that are finished
  for (i = 0; i < xbt_dynar_length(to_free); i++) {
    w = NULL;
    w = xbt_dynar_get_ptr(to_free, i);
    if ((*w)->state == DONE) {
      //printf("[%lf]must delete %s\n", MSG_get_clock(), (*w)->name);
      xbt_free((*w)->mapping);
      xbt_free((*w));
      xbt_dynar_remove_at(to_free, i, NULL);
      i--;
    }
  }
}



static void remove_job(xbt_dynar_t list, job_t job) {
  unsigned int i = 0;
  job_t * j;
  for (i = 0; i < xbt_dynar_length(list); i++) {
    j = xbt_dynar_get_ptr(list, i);
    if (((*j)->user_id == job->user_id) &&
        ((*j)->id == job->id)) {
      xbt_dynar_remove_at(list, i, NULL);
      return;
    }
  }
}
