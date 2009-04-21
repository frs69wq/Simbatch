#include "sed.h"
#include "defs.h"
#include <xbt.h>
#include "myutils.h"
#include <float.h>
#include <stdio.h>
#include <msg/msg.h>

static winner_t * get_job(m_host_t host, m_host_t batch, 
			  SG_job_t SGjob, performances* perfs);
static void update_jobs(xbt_dynar_t jobs, xbt_dynar_t to_free);
static void remove_job(xbt_dynar_t list, job_t job);
static performances * parse_perf_file(const char * file_name);
static void free_performances(performances * perfs);

int sed(int argc, char ** argv) {
  m_task_t task;
  m_host_t batch;
  m_host_t metaSched;
  MSG_error_t err;
  xbt_fifo_t msg_stack;
  xbt_dynar_t jobs;
  xbt_dynar_t to_free;
  char * perf_file;
  performances * perfs;

  if (argc < 4) {
    printf("Sed badly defined in deployment file\n");
    exit(EXIT_FAILURE);
  }

  printArgs(argc, argv);

  metaSched = MSG_get_host_by_name(argv[1]);
  batch = MSG_get_host_by_name(argv[2]);
  perf_file = argv[3];
  perfs = parse_perf_file(perf_file);

  //sed receives tasks
  msg_stack = xbt_fifo_new();
  jobs = xbt_dynar_new(sizeof(job_t*), NULL);
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
        winner_t * w = get_job(MSG_host_self(), batch, 
			       (SG_job_t)MSG_task_get_data(task), 
			       perfs);
	MSG_task_put(MSG_task_create("SB_RESP_SCHED", 0, 0, w), 
		     metaSched, MS_CHAN);
      }//SB_ASK_SCHED


      else if (strcmp(task->name, "SB_TASK") == 0) {
        job_t job = MSG_task_get_data(task);
        xbt_dynar_push(jobs, &job);
        MSG_task_put(MSG_task_create("SB_TASK", 0, job->input_size, job),
                                     batch, CLIENT_PORT);
      }//SB_TASK


      else if (strcmp(task->name, "SB_TASK_CANCEL") == 0) {
        job_t job;
        job = ((cancelation*)MSG_task_get_data(task))->job;
        remove_job(jobs, job);
	MSG_task_put(MSG_task_create("SB_TASK_CANCEL", 0, 0, job), 
                     batch, CLIENT_PORT);
	m_task_t task = NULL;
	MSG_task_get(&task, SED_CHANNEL);
	if (strcmp(MSG_task_get_name(task), "CANCEL_OK") == 0) {
#ifdef VERBOSE
	  printf("[%lf]%s canceled on batch %s\n", MSG_get_clock(), 
		 job->name, MSG_host_get_name(MSG_host_self()));
#endif
	  MSG_task_put(MSG_task_create("CANCEL_OK", 0, 0, NULL), 
		       metaSched, MS_CANCEL_CHAN);
	}
	else {
	  MSG_task_put(MSG_task_create("CANCEL_KO", 0, 0, NULL), 
		       metaSched, MS_CANCEL_CHAN);
	}
	MSG_task_destroy(task);
      }//SB_TASK_CANCEL


      else if (strcmp(task->name, "SB_TRANSMIT_TASK") == 0) {
        job_t job;
        job = ((cancelation*)MSG_task_get_data(task))->job;
        m_host_t host;
        host = ((cancelation*)MSG_task_get_data(task))->host;

        if (host != NULL) {
	  if (strcmp(MSG_host_get_name(MSG_host_self()), MSG_host_get_name(host)) == 0) {
	    printf("source = destination pour %s\n", job->name);
	  }
          remove_job(jobs, job);
          MSG_task_put(MSG_task_create("SB_TASK", 0, job->input_size, job),
                       host, SED_CHAN);
#ifdef VERBOSE
          printf("[%lf]transmitted %s from %s to %s\n", 
                 MSG_get_clock(), job->name,
                 MSG_host_get_name(MSG_host_self()), 
                 MSG_host_get_name(host));//*/
#endif
        }
        else {
          MSG_task_put(MSG_task_create("SB_TASK", 0, job->input_size, job),
                                     batch, CLIENT_PORT);
#ifdef VERBOSE
          printf("[%lf]%s resubmited to %s\n", 
		 MSG_get_clock(), job->name, 
		 MSG_host_get_name(MSG_host_self()));//*/
#endif
        }
	MSG_task_put(MSG_task_create("OK", 0, 0, NULL), 
		     metaSched, MS_CHAN);
      }//SB_TRANSMIT_TASK


      else if (strcmp(task->name, "GET_JOBS") == 0) {
        update_jobs(jobs, to_free);
        xbt_dynar_t wins = xbt_dynar_new(sizeof(cancelation**), NULL);
        unsigned int i;
        cancelation ** can;
        for (i = 0; i < xbt_dynar_length(jobs); i++) {
          can = (cancelation**)malloc(sizeof(cancelation*));
          *can = (cancelation*)malloc(sizeof(cancelation));
          (*can)->job = *((job_t*)xbt_dynar_get_ptr(jobs, i));
          (*can)->host = MSG_host_self();
	  xbt_dynar_push(wins, can);
          can = NULL;
        }
        MSG_task_put(MSG_task_create("GET_JOBS", 0, 0, wins),
                     MSG_task_get_source(task), MS_JOB_CHAN);//*/
      }//GET_JOBS


      else if (strcmp(task->name, "DO_NOTHING") == 0) {
      } //DO_NOTHING

#ifdef TRANSFERT_TIME
      else if (strcmp(task->name, "ASK_ESTIMATED_TRANSFERT_TIME") == 0) {
        double * time;
        double time1;
        cancelation * can;
        time = (double*) malloc(sizeof(double));
        
        can = MSG_task_get_data(task);
	time1 = MSG_get_clock();
        MSG_task_put(MSG_task_create("DO_NOTHING", 0,
                                     can->job->input_size, NULL),
                     can->host, SED_CHAN);
        *time = MSG_get_clock() - time1;
        MSG_task_put(MSG_task_create("RESPONSE_ESTIMATED_TIME", 0, 0, time),
		     MSG_task_get_source(task), MS_CHAN);
      } //ASK_ESTIMATED_TRANSFERT_TIME
#endif

      else {
        printf("Task unrecognized. Destroying it\n");
      }//Unknown task

      
      MSG_task_destroy(task);
      task = NULL;
    }
  }
  
#ifdef VERBOSE
  printf("Sed on %s exited main loop\n", MSG_host_get_name(MSG_host_self()));
#endif
  free_performances(perfs);
  xbt_fifo_free(msg_stack);
  xbt_dynar_free(&jobs);
  xbt_dynar_free(&to_free);
  
  return EXIT_SUCCESS;
}


static void free_performances(performances * perfs) {
  int i;
  for (i = 0; i < perfs->nbP; i++){
    xbt_free(perfs->p[i]);
  }
  xbt_free(perfs->p);
  xbt_free(perfs->sizes);
  xbt_free(perfs);
}

/*static perf * parse_perf_file(const char * file_name, const int service, 
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


  while (fgets(buf,512,f) != NULL) {
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
          fclose(f);
          printf("Speedup is known. Does nothing for now\n");
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
	    results[i].service = service;
            sscanf(buf, "%d %lf", 
                   &(results[i].nb_proc), &(results[i].time));
          }
          fclose(f);
          *result_size = nb_type;
          return results;
        }
      }
    }
  }
  fclose(f);
  *result_size = -1;
  return NULL;
}
*/
static performances * parse_perf_file(const char * file_name) {
  FILE * f = NULL;
  char buf[512];
  unsigned int type = 0;
  int nb_type;
  int i = 0;
  int know_speedup;
  unsigned long int ref_power;
  performances * result;
  int nb_services = 0;
  int current_perf = 0;

#ifdef VERBOSE
  printf("Parsing %s...", file_name);
#endif

  f = fopen(file_name, "r");
  
  if (f == NULL) {
    fprintf (stderr, "%s:line %d, function %s, fopen failed : %s \n",   \
             __FILE__, __LINE__, __func__, file_name);
    return NULL;
  }

  //count the number of services
  while (fgets(buf,512,f) != NULL) {
    if (buf[0] != ';' && buf[0] != '#') {
      sscanf(buf, "%u %d", &type, &nb_type);
      //if we found a wrong service, we go to the next
      for (i = 0; i <= nb_type; i++) {
	if (fgets(buf,512,f) == NULL) {
	  exit(EXIT_FAILURE);
	}
      }
      nb_services++;
    }
  }
  rewind(f);

  result = (performances*)xbt_malloc(sizeof(performances));
  result->nbP = nb_services;
  result->p = (perf**)xbt_malloc(sizeof(perf*) * nb_services);
  result->sizes = (int*)xbt_malloc(sizeof(int) * nb_services);

  while (fgets(buf,512,f) != NULL) {
    if (buf[0] != ';' && buf[0] != '#') {
      sscanf(buf, "%u %d", &type, &nb_type);
      if (fgets(buf, 512, f) == NULL) {
	exit(EXIT_FAILURE);
      }
      sscanf(buf, "%d %lu", &know_speedup, &ref_power);
      if (know_speedup) {
	result->sizes[current_perf] = 0;
	result->p[current_perf] = NULL;
      }
      else { 	
	result->p[current_perf] = (perf*)xbt_malloc(nb_type * sizeof(perf));
	for (i = 0; i < nb_type; i++) {
	  if (fgets(buf, 512, f) == NULL) {
	    exit(EXIT_FAILURE);
	  }
	  result->p[current_perf][i].reference_power = ref_power;
	  result->p[current_perf][i].service = type;
	  unsigned int nbp;
	  double t;
	  sscanf(buf, "%u %lf", &nbp, &t); 
	  result->p[current_perf][i].nb_proc = nbp; 
	  result->p[current_perf][i].time = t;
	}
	result->sizes[current_perf] = nb_type;
      }
      current_perf++;
    }
  }
  fclose(f);

#ifdef VERBOSE
  printf("OK\n");
#endif

  return result;
}

static perf * get_perf(performances * performances, unsigned int service, 
		       int * res_size) {
  int i;
  for (i = 0; i < performances->nbP; i++) {
    if ((performances->p[i])->service == service) {
      *res_size = performances->sizes[i];
      return performances->p[i];
    }
  }
  *res_size = -1;
  return NULL;
}

/*
 *Called by the SeD to choose on how many processors to execute a task
 */
static winner_t * get_job(m_host_t host, m_host_t batch, 
			  SG_job_t SGjob, 
			  performances * perfor) {
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

  perfs = get_perf(perfor, SGjob->service, &res_size);

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
        perfs = xbt_malloc(sizeof(perf));
        perfs[0].nb_proc = a;
        perfs[0].reference_power = b;
        perfs[0].time = c;
        found = 1;
      }
      i++;
    }
    
    if (!found) {
      res_size = -2;
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
    printf("[%lf]Service not found for %s on %s\n", MSG_get_clock(),
	   SGjob->name, MSG_host_get_name(MSG_host_self()));
    return NULL;
  }

  //if the number of processors asked is not valid
  if (res_size == -2) {
    //create the winner with a CT of -1
    printf("[%lf]%d processors on %s is not in perf file for %s\n", 
	   MSG_get_clock(), SGjob->nb_procs, 
	   MSG_host_get_name(MSG_host_self()), SGjob->name);
    return NULL;
  }

  //in the other cases, try every possibility of number of processors.
  //get the completion time for each number of processors
  double completionT;
  slot_t * slots;
  failures = 0;
  for (i = 0; i < res_size; i++) {
    job->run_time = (perfs[i].reference_power / MSG_get_host_speed(host)) 
      * perfs[i].time;
    
    job->wall_time = job->run_time * SGjob->wall_time;
        
    job->nb_procs = perfs[i].nb_proc;
    MSG_task_put(MSG_task_create("SED_PRED", 0, 0, job), 
                 batch, CLIENT_PORT);
    task = NULL;
    MSG_task_get(&task, SED_CHANNEL);
    if (strcmp(task->name, "SB_PRED") == 0) { // OK
      slots = NULL;
      slots = MSG_task_get_data(task);
      completionT = slots[job->nb_procs - 1]->start_time + job->wall_time;
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
    printf("[%lf]%s unable to execute job %s (not enough processors)\n", 
	   MSG_get_clock(), MSG_host_get_name(MSG_host_self()), 
	   job->name);//*/
    winner->completionT = -1;
  }

  if (SGjob->nb_procs > 0) {
    xbt_free(perfs);
  }

  return winner;
}


void update_jobs(xbt_dynar_t jobs, xbt_dynar_t to_free) {
  unsigned int i;
  job_t * w;
  //tests if the jobs that have been submitted are still waiting
  for (i = 0; i < xbt_dynar_length(jobs); i++) {
    w = NULL;
    w = xbt_dynar_get_ptr(jobs, i);
    if ((*w)->state != WAITING) {
      xbt_dynar_push(to_free, w);
      w = NULL;
      xbt_dynar_remove_at(jobs, i, w);
      i--;
    }
  }
  
  //free jobs that are finished
  for (i = 0; i < xbt_dynar_length(to_free); i++) {
    w = NULL;
    w = xbt_dynar_get_ptr(to_free, i);
    if ((*w)->state == DONE) {
      xbt_free((*w)->mapping);
      xbt_free((*w)->data);
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
