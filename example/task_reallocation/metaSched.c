#include "metaSched.h"
#include "defs.h"
#include <msg/msg.h>
#include "myutils.h"
#include <float.h>
#include <stdio.h>
#include <xbt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

static winner_t * MCT_Schedule(m_task_t task, m_host_t * cluster, 
                               int nbClusters, m_host_t source);
#ifdef MINMIN
static winner_t * MinMin_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				  int nbClusters, int * position);
#endif
#ifdef MAXMIN
static winner_t * MaxMin_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				  int nbClusters, int * position);
#endif
#ifdef MAXGAIN
static winner_t * MaxGain_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				   int nbClusters, int * position);
#endif
static xbt_dynar_t sort_jobs(int nbClusters, xbt_dynar_t jobs[]);
#ifdef TRANSFERT_TIME
static void estimate_transfert_time(double temps[], m_task_t task, 
				    m_host_t * cluster, 
				    int nbClusters, m_host_t source);
#endif
static int cmp_cancelations(const void *p1, const void *p2);

int resched(int argc, char ** argv) {
#ifdef RESCHED
  double step;
  double max;
  double time = 0.0;
  
  if (argc < 3) {
    return EXIT_SUCCESS;
  }

  step = atof(argv[1]);
  max = atoi(argv[2]);
  while (time < max) {
    MSG_process_sleep(step);
    time += step;
    MSG_task_put(MSG_task_create("SB_ASK_RESCHED", 0, 0, NULL), 
                 MSG_host_self(), MS_CHAN);
  }
#endif
  return EXIT_SUCCESS;
}

int metaSched(int argc, char ** argv) {
  m_task_t task;
  MSG_error_t err;
  xbt_fifo_t msg_stack;
  int nbClusters = argc - 2;
  m_host_t clusters[nbClusters];
  m_host_t client;
  int i;
  FILE * realloc_file;
  realloc_file = fopen("realloc.txt", "w");

  printArgs(argc, argv);

  for (i = 0; i < nbClusters; i++) {
    clusters[i] = MSG_get_host_by_name(argv[i+2]);
  }
  client = MSG_get_host_by_name(argv[1]);
  
  msg_stack = xbt_fifo_new();
  while (1) {  

    task = NULL;
    err = MSG_task_get_with_time_out(&task, MS_CHAN, DBL_MAX);
    if (err == MSG_TRANSFER_FAILURE) { 
      break; 
    }
    if (err == MSG_OK) {
      xbt_fifo_push(msg_stack, task);
      while (MSG_task_Iprobe(MS_CHAN)) {
	task = NULL;
	MSG_task_get(&task, MS_CHAN);
	xbt_fifo_push(msg_stack, task);
      }
    }

    while (xbt_fifo_size(msg_stack)) {
      
      task = xbt_fifo_shift(msg_stack);

      if (strcmp(task->name, "SB_ASK_SCHED") == 0) {
        winner_t * winner = MCT_Schedule(task, clusters, nbClusters, 
					 MSG_task_get_source(task));
        MSG_task_put(MSG_task_create("SB_RESULT_SCHED", 0, 0, winner),
                     client, CLIENT_CHAN);
      }//SB_ASK_SCHED

      if (strcmp(task->name, "SB_ASK_RESCHED") == 0) {
	int something_happened = 0;
        m_task_t tmp;
        xbt_dynar_t list;
        xbt_dynar_t jobs[nbClusters];
        for (i = 0; i < nbClusters; i++) {
          MSG_task_put(MSG_task_create("GET_JOBS", 0, 0, NULL), 
                       clusters[i], SED_CHAN);
          tmp = NULL;
          MSG_task_get(&tmp, MS_JOB_CHAN);
          jobs[i] = (xbt_dynar_t)MSG_task_get_data(tmp);
          MSG_task_destroy(tmp);
        }

        list = sort_jobs(nbClusters, jobs);

	struct timeval tv;
	time_t curtimeu;
	time_t curtime;
	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;
	curtimeu=tv.tv_usec;
	int jobsArescheduler = xbt_dynar_length(list);

        cancelation ** tmp1;
	int position = 0;
        winner_t * newWinner;
	while (xbt_dynar_length(list) > 0) {
#ifdef MCT
	  tmp1 = xbt_dynar_get_ptr(list, 0);
          newWinner = MCT_Schedule(MSG_task_create("SB_ASK_SCHED", 0, 0, 
						   (*tmp1)->job->data), 
				   clusters, nbClusters, (*tmp1)->host);//*/
#endif
#ifdef MINMIN
	  newWinner = MinMin_Schedule(list, clusters, nbClusters, &position);
	  tmp1 = xbt_dynar_get_ptr(list, position);
#endif
#ifdef MAXMIN
	  newWinner = MaxMin_Schedule(list, clusters, nbClusters, &position);
	  tmp1 = xbt_dynar_get_ptr(list, position);
#endif
#ifdef MAXGAIN
	  newWinner = MaxGain_Schedule(list, clusters, nbClusters, &position);
	  tmp1 = xbt_dynar_get_ptr(list, position);
#endif
          if (newWinner->completionT < (*tmp1)->job->completion_time &&
	      strcmp(newWinner->cluster->name, (*tmp1)->host->name) != 0) {
            cancelation * can;
            can = (cancelation*)xbt_malloc(sizeof(cancelation));
            can->job = (*tmp1)->job;
            can->host = newWinner->cluster;
	    
            MSG_task_put(MSG_task_create("SB_TASK_CANCEL", 0, 0, can),
                         (*tmp1)->host, SED_CHAN);
	    m_task_t t = NULL;
	    MSG_task_get(&t, MS_CANCEL_CHAN);
	    if (strcmp(MSG_task_get_name(t), "CANCEL_OK") == 0) {
	      MSG_task_destroy(t);
	      //ask if it is better to resubmit to the same batch, and
	      //resubmit if it is the case
	      m_task_t t1 = NULL;
	      MSG_task_put(MSG_task_create("SB_ASK_SCHED", 0, 0, 
					   can->job->data),
			   (*tmp1)->host, SED_CHAN);
	      t1 = NULL;
	      MSG_task_get(&t1, MS_CHAN);
	      winner_t * w1 = (winner_t*)MSG_task_get_data(t1);
	      MSG_task_destroy(t1);

	      if (w1->completionT < newWinner->completionT) {
#ifdef VERBOSE
		printf("[%lf]%s has no need to change batch(%lf vs %lf)\n", 
		       MSG_get_clock(), can->job->name, w1->completionT, 
		       newWinner->completionT);//*/
#endif
		can->host = NULL;
	      }
	      else {
#ifdef VERBOSE
		printf("[%lf]%s is on %s (%lf), "
		       "it will be transmitted to %s (%lf)\n",
		       MSG_get_clock(), can->job->name, 
		       MSG_host_get_name((*tmp1)->host),
		       (*tmp1)->job->completion_time, 
		       MSG_host_get_name(can->host),
		       newWinner->completionT);
#endif
		fprintf(realloc_file, "[%lf]%s is on %s (%lf), "
			"it will be transmitted to %s (%lf)\n",
			MSG_get_clock(), can->job->name, 
			MSG_host_get_name((*tmp1)->host),
			(*tmp1)->job->completion_time, 
			MSG_host_get_name(can->host),
			newWinner->completionT);
		fflush(realloc_file);
	      }

	      MSG_task_put(MSG_task_create("SB_TRANSMIT_TASK", 0, 0, can),
			   (*tmp1)->host, SED_CHAN);
	      t1 = NULL;
	      MSG_task_get(&t1, MS_CHAN);
	      MSG_task_destroy(t1);
	      can = NULL;
	      xbt_free(can);
	      something_happened = 1;
	    }
	  }
	  xbt_dynar_remove_at(list, position, NULL);
	}
	xbt_dynar_free(&list);
#ifdef VERBOSE
	if (something_happened) {
	  printf("\n");
	}
#endif

	if (jobsArescheduler != 0) {
	  gettimeofday(&tv, NULL);
	  FILE * f = fopen("temps.txt", "a");
	  fprintf(f, "%lf\n", 
		  (tv.tv_sec - curtime) + (tv.tv_usec - curtimeu) / 1000000.0);
	  fclose(f);
	}
      }//SB_ASK_RESCHED

      MSG_task_destroy(task);
      task = NULL;
    }
  }
#ifdef VERBOSE  
  printf("MetaSched exited main loop\n");
#endif

  fclose(realloc_file);

  xbt_fifo_free(msg_stack);

  return EXIT_SUCCESS;
}

#ifdef TRANSFERT_TIME
static void estimate_transfert_time(double temps[], m_task_t task, 
				    m_host_t * cluster, 
				    int nbClusters, m_host_t source) {
  /*
    Computation of transfert times (from source to cluster[i]: 
    (if it comes from the client, the time is set to 0)
   */
  if (strcmp(source->name, "Client") != 0) {
    int i;
    cancelation * estim;
    job_t job;
    job = xbt_malloc(sizeof(*job));
    job->input_size = ((SG_job_t)MSG_task_get_data(task))->input_size; 
    strcpy(job->name, ((SG_job_t)MSG_task_get_data(task))->name); 
    estim = (cancelation*)xbt_malloc(sizeof(cancelation));
    estim->job = job;
    for (i = 0; i < nbClusters; i++) {
      estim->host = cluster[i];
      if (strcmp(source->name, cluster[i]->name) != 0) {
	
	pid_t pid;
	int pfd[2];
	FILE* file;
	if (pipe(pfd) == -1) {
	  printf("Error on pipe creation\n");
	  exit(EXIT_FAILURE);
	}
	
	pid = fork();
	if (pid == 0) {
	  double d;
	  MSG_task_put(MSG_task_create("ASK_ESTIMATED_TRANSFERT_TIME", 
				       0, 0, estim), 
		       source, SED_CHAN);
	  m_task_t result = NULL;
	  MSG_task_get(&result, MS_CHAN);
	  d = *((double*)result->data);
	  close(pfd[0]);
	  file = fdopen(pfd[1], "w");
	  fprintf(file, "%lf", d);
	  fclose(file);
	  exit(EXIT_SUCCESS);
	}
	else if (pid < 0) {
	  printf("Erreur dans le fork\n");
	  exit(EXIT_FAILURE);
	}
	else {
	  int status;
	  double d;
	  waitpid(pid, &status, 0);
	  close(pfd[1]);
	  file = fdopen(pfd[0], "r");
	  status = fscanf(file, "%lf", &d);
	  fclose(file);
	  temps[i] = d;
	}
      }
    }
  }
}
#endif


/*merge all the jobs in a single array and sort this array with a metric*/
static xbt_dynar_t sort_jobs(int nbClusters, xbt_dynar_t jobs[]) {
  int i;
  unsigned int j;
  int nbElts;
  cancelation ** w;
  xbt_dynar_t list;
  cancelation** tab_temp;

  list = xbt_dynar_new(sizeof(cancelation**), NULL);

  nbElts = 0;
  for (i = 0; i < nbClusters; i++) {
    nbElts += xbt_dynar_length(jobs[i]);
  }
  
  tab_temp = (cancelation**)xbt_malloc(sizeof(cancelation*) * nbElts);
  unsigned int l;
  int counter = 0;
  for (i = 0; i < nbClusters; i++) {
    l = xbt_dynar_length(jobs[i]);
    for (j = 0; j < l; j++) {
      w = (cancelation**) xbt_dynar_get_ptr(jobs[i], 0);
      tab_temp[counter] = *w;
      w = NULL;
      xbt_dynar_remove_at(jobs[i], 0, w);
      counter++;
    }
  }

  qsort(tab_temp, (size_t)nbElts, sizeof(cancelation*), cmp_cancelations);

  for (i = 0; i < nbElts; i++) {
    xbt_dynar_push(list, &tab_temp[i]);
  }

  xbt_free(tab_temp);

  for (i = 0; i < nbClusters; i++) {
    xbt_dynar_free(&(jobs[i]));
  }
  
  return list;
}

static int cmp_cancelations(const void *p1, const void *p2) {
  if ((*((cancelation**)p1))->job->submit_time < (*((cancelation**)p2))->job->submit_time) {
    return -1;
  }
  if ((*((cancelation**)p1))->job->submit_time > (*((cancelation**)p2))->job->submit_time) { 
    return 1;
  }
  return 0;
}


static winner_t * MCT_Schedule(m_task_t task, m_host_t * cluster, 
                               int nbClusters, m_host_t source) {
  int i;
  winner_t ** winners;
  winner_t * winner = NULL;
  double bestCT = DBL_MAX;
  m_task_t task_temp = NULL;
  int bestI = -1;
  double temps[nbClusters];
  
  for (i = 0; i < nbClusters; i++) {
    temps[i] = 0;
  }

  //ask when a task could be completed
  winners = xbt_malloc(nbClusters * sizeof(winner_t*));
  //send the task to ALL the seds
  for (i = 0; i < nbClusters; i++) {
    //printf("[%lf]MCT_schedule: Avant SB_ASK_SCHED sur %s\n", MSG_get_clock(), cluster[i]->name);
    MSG_task_put(MSG_task_create("SB_ASK_SCHED", 0, 0, task->data), 
                 cluster[i], SED_CHAN);
    task_temp = NULL;
    MSG_task_get(&task_temp, MS_CHAN);
    //printf("[%lf]MCT_schedule: Apres SB_ASK_SCHED\n", MSG_get_clock());
    
    winners[i] = MSG_task_get_data(task_temp);
    MSG_task_destroy(task_temp);
    task_temp = NULL;
  }

#ifdef TRANSFERT_TIME
  //estimate transfert times
  estimate_transfert_time(temps, task, cluster, nbClusters, source);
#endif


  //compute MCT
  for (i = 0; i < nbClusters; i++) {
    if (winners[i] != NULL && winners[i]->completionT != -1) {
      /*printf("temps de %s a %s pour %s: %lf\n", source->name,
	     winners[i]->cluster->name, winners[i]->job->name, temps[i]);*/
      winners[i]->completionT += temps[i];
      if (winners[i]->completionT< bestCT) {
        winner = winners[i];
        bestCT = winners[i]->completionT;
        if (bestI != -1) {
          xbt_free(winners[bestI]);
        }
        bestI = i;
      }
      else {
        xbt_free(winners[i]);
      }
    }
  }
  xbt_free(winners);

#ifdef VERBOSE
  if (winner == NULL) {
    printf("[%lf]No batch is able to execute %s\n", 
           MSG_get_clock(), ((SG_job_t)task->data)->name); 
  }
#endif

  return winner;
}

#ifdef MINMIN
static winner_t * MinMin_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				  int nbClusters, int * position) {
  int i;
  double minCT;
  int nb_jobs = xbt_dynar_length(job_list);
  winner_t * winner = NULL;
  winner_t * winner_temp;
  cancelation ** tmp1;

  minCT = DBL_MAX;
  for (i = 0; i < nb_jobs; i++) {
    tmp1 = (cancelation**)xbt_dynar_get_ptr(job_list, i);
    winner_temp = MCT_Schedule(MSG_task_create("SB_ASK_SCHED", 0, 0, 
					       (*tmp1)->job->data), 
			       cluster, nbClusters, (*tmp1)->host);
    if (winner_temp->completionT < minCT) {
      if (winner != NULL) {
	xbt_free(winner);
      }
      winner = winner_temp;
      *position = i;
      minCT = winner->completionT;
    }
    else {
      xbt_free(winner_temp);
    }
  }
  
  return winner;
}
#endif


#ifdef MAXMIN
static winner_t * MaxMin_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				  int nbClusters, int * position) {
  int i;
  double maxCT;
  int nb_jobs = xbt_dynar_length(job_list);
  winner_t * winner = NULL;
  winner_t * winner_temp;
  cancelation ** tmp1;

  maxCT = 0;
  for (i = 0; i < nb_jobs; i++) {
    tmp1 = (cancelation**)xbt_dynar_get_ptr(job_list, i);
    winner_temp = MCT_Schedule(MSG_task_create("SB_ASK_SCHED", 0, 0, 
					       (*tmp1)->job->data), 
			       cluster, nbClusters, (*tmp1)->host);
    if (winner_temp->completionT > maxCT) {
      if (winner != NULL) {
	xbt_free(winner);
      }
      winner = winner_temp;
      *position = i;
      maxCT = winner->completionT;
    }
    else {
      xbt_free(winner_temp);
    }
  }
  
  return winner;
}
#endif

#ifdef MAXGAIN
static winner_t * MaxGain_Schedule(xbt_dynar_t job_list, m_host_t * cluster, 
				   int nbClusters, int * position) {
  printf("debut\n");
  int i;
  double gain;
  int nb_jobs = xbt_dynar_length(job_list);
  winner_t * winner = NULL;
  winner_t * winner_temp;
  cancelation ** tmp1;

  gain = -1.0;
  for (i = 0; i < nb_jobs; i++) {
    tmp1 = (cancelation**)xbt_dynar_get_ptr(job_list, i);
    winner_temp = MCT_Schedule(MSG_task_create("SB_ASK_SCHED", 0, 0, 
					       (*tmp1)->job->data), 
			       cluster, nbClusters, (*tmp1)->host);
    printf("[%lf]%s\n", MSG_get_clock(), winner_temp->job->name);
    printf("gain: %lf - %lf = %lf\n", winner_temp->completionT, 
	     (*tmp1)->job->completion_time,
	     winner_temp->completionT - (*tmp1)->job->completion_time);
    if (winner_temp->completionT - (*tmp1)->job->completion_time > gain) {
      if (winner != NULL) {
	xbt_free(winner);
      }
      winner = winner_temp;
      *position = i;
      gain = winner->completionT - (*tmp1)->job->completion_time;
    }
    else {
      xbt_free(winner_temp);
    }
  }
  if (winner == NULL) {
    printf("winner NULL\n");
  }
  printf("fin\n");
  return winner;
}
#endif
