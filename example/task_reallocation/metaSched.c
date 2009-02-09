#include "metaSched.h"
#include "defs.h"
#include <msg/msg.h>
#include <stdlib.h>
#include "myutils.h"
#include <float.h>
#include <stdio.h>
#include <xbt.h>

static winner_t * MCT_Schedule(m_task_t task, m_host_t * cluster, 
                               int nbClusters);
static xbt_dynar_t sort_jobs(int nbClusters, xbt_dynar_t jobs[]);

int resched(int argc, char ** argv) {

  double step;
  double time = 0.0;
  
  if (argc < 2) {
    return EXIT_SUCCESS;
  }

  step = atof(argv[1]);
  while (time < 16000) {
    MSG_process_sleep(step);
    time += step;
    MSG_task_put(MSG_task_create("SB_ASK_RESCHED", 0, 0, NULL), 
                 MSG_host_self(), MS_CHAN);
  }//*/

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
  winner_t * winner;

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
        winner = MCT_Schedule(task, clusters, nbClusters);
        if (winner == NULL) {
          printf("No winner found\n");
        }
        MSG_task_put(MSG_task_create("SB_RESULT_SCHED", 0, 0, winner),
                     client, CLIENT_CHAN);
      }//SB_ASK_SCHED

      if (strcmp(task->name, "SB_ASK_RESCHED") == 0) {
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

        cancelation ** tmp1;
        winner_t * newWinner;
	while (xbt_dynar_length(list) > 0) {
	  tmp1 = xbt_dynar_get_ptr(list, 0);
          
          newWinner = MCT_Schedule(MSG_task_create("SB_ASK_SCHED", 0, 0, (*tmp1)->job->data), clusters, nbClusters);

          /////////////////////////////////////////////
          //faudrait prendre le temps de comm en compte
          /////////////////////////////////////////////
          
          if (newWinner->completionT < (*tmp1)->job->completion_time) {
            cancelation * can;
            can = (cancelation*)malloc(sizeof(cancelation));
            can->job = (*tmp1)->job;
            can->host = newWinner->cluster;

            MSG_task_put(MSG_task_create("SB_TASK_CANCEL", 0, 0, can),
                         (*tmp1)->host, SED_CHAN);

            m_task_t t1 = NULL;
            MSG_task_put(MSG_task_create("SB_ASK_SCHED", 0, 0, can->job->data),
                         (*tmp1)->host, SED_CHAN);
            t1 = NULL;
            MSG_task_get(&t1, MS_CHAN);
            winner_t * w1 = (winner_t*)MSG_task_get_data(t1);
            MSG_task_destroy(t1);

            if (w1->completionT < newWinner->completionT) {
              
              printf("pas besoin de changer %s de place(%lf, %lf)\n", 
                     can->job->name, w1->completionT, 
                     newWinner->completionT);
              printf("proc: %d %d\n", w1->job->nb_procs, newWinner->job->nb_procs);
              printf("start: %lf %lf\n", w1->job->start_time, newWinner->job->start_time);
              can->host = NULL;
            }//*/
            MSG_task_put(MSG_task_create("SB_TRANSMIT_TASK", 0, 0, can),
                         (*tmp1)->host, SED_CHAN);
            can = NULL;
          }
          xbt_dynar_remove_at(list, 0, NULL);
	}
        
	xbt_dynar_free(&list);
      }//SB_ASK_RESCHED

      MSG_task_destroy(task);
      task = NULL;
    }
  }
  
  printf("MetaSched exited main loop\n");

  xbt_fifo_free(msg_stack);

  return EXIT_SUCCESS;
}



static winner_t * MCT_Schedule(m_task_t task, m_host_t * cluster, 
                               int nbClusters) {
  int i;
  winner_t ** winners;
  winner_t * winner = NULL;
  double bestCT = DBL_MAX;
  m_task_t task_temp = NULL;
  int bestI = -1;

  winners = xbt_malloc(nbClusters * sizeof(winner_t*));
  //send the task to ALL the seds
  for (i = 0; i < nbClusters; i++) {
    MSG_task_put(MSG_task_create("SB_ASK_SCHED", 0, 0, task->data), 
                 cluster[i], SED_CHAN);
    task_temp = NULL;
    MSG_task_get(&task_temp, MS_CHAN);
    
    winners[i] = MSG_task_get_data(task_temp);
    MSG_task_destroy(task_temp);
    task_temp = NULL;
  }

  for (i = 0; i < nbClusters; i++) {
    if (winners[i] != NULL && winners[i]->completionT != -1) {
      if (winners[i]->completionT < bestCT) {
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
  
  if (winner == NULL) {
    printf("No batch is able to execute %s\n", 
           ((SG_job_t)task->data)->name); 
  }
  return winner;
}






static xbt_dynar_t sort_jobs(int nbClusters, xbt_dynar_t jobs[]) {
  int i;
  unsigned int j;
  int bestI;
  int bestJ;
  int nbElts;
  cancelation ** w;
  cancelation ** best = NULL;
  xbt_dynar_t list;
  double min;

  list = xbt_dynar_new(sizeof(cancelation**), NULL);
  //w = (winner_t**)xbt_malloc(sizeof(winner_t*));
  //best = (winner_t**)xbt_malloc(sizeof(winner_t*));

  nbElts = 0;
  for (i = 0; i < nbClusters; i++) {
    nbElts += xbt_dynar_length(jobs[i]);
  }

  while (nbElts > 0) {
    bestI = 0;
    bestJ = 0;
    min = DBL_MAX;
    for (i = 0; i < nbClusters; i++) {
      for (j = 0; j < xbt_dynar_length(jobs[i]); j++) {
        w = xbt_dynar_get_ptr(jobs[i], j);
        if ((*w)->job->submit_time < min) {
          best = w;
          bestI = i;
          bestJ = j;
          min = (*w)->job->submit_time;
        }
      }
    }
    xbt_dynar_push(list, best);
    //printf("%lf\n", (*best)->completionT);
    w = NULL;
    //printf("removing %d %d from %d elements\n", bestI, bestJ, nbElts);
    xbt_dynar_remove_at(jobs[bestI], bestJ, w);
    //printf("%s %lf\n", (*best)->job->name, (*best)->job->submit_time);
    nbElts--;
  }

  for (i = 0; i < nbClusters; i++) {
    xbt_dynar_free(&(jobs[i]));
  }
  
  return list;
}

