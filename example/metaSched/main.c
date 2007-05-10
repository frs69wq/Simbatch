/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project get_name : SimBatch     */
/*******************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <xbt/fifo.h>
#include <xbt/sysdep.h>
#include <msg/msg.h>

#include "simbatch.h"
#include "algos.h"
#include "parser.h"


void printArgs(int argc, char **argv);

int isAvailable(const char * services[], const char * service);

double str2double(const char * str);


void printArgs(int argc, char ** argv) {
    int i=1;
    for (i=1; i<argc; ++i) {
        printf("\t%2d - %s\n", i, argv[i]); 
    }
}


double str2double(const char * str) {
    char * end;
    double val = strtod(str, &end);

    if (end == str) { return 0; }
    if (((val == DBL_MAX) || (val == DBL_MIN)) && (errno == ERANGE)) {
        return 0;
    }
    return val;
}


int isAvailable(const char * services[], const char * service) {
    int i = 0, found = 0;
    while (services[i] && !found) {
        if (!strcmp(services[i], service)) { found = 1; }
        else { ++i; }
    }
    return found;
}


/* Ratio between clusters */
double * getSpeedCoef(const m_host_t * clusters, const int nbClusters) {
    double * speedCoef = NULL;
    double maxSpeed = 0;
    int i=0;

    speedCoef = xbt_malloc(sizeof(double) * nbClusters);

    for (i=0; i<nbClusters; ++i) {
        if (maxSpeed < MSG_get_host_speed(clusters[i]))
            maxSpeed = MSG_get_host_speed(clusters[i]);
    }

    for (i=0; i<nbClusters; ++i) {
        speedCoef[i] = maxSpeed / MSG_get_host_speed(clusters[i]);
    }

    return speedCoef;
}




int metaSched(int argc, char ** argv) {
    const int nbClusters = argc - 1;
    m_host_t clusters[nbClusters];
    xbt_fifo_t jobList = NULL;
    winner_t * winner = NULL;
    double * speedCoef = NULL;
    int i = 1;

    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);

    for (i=1; i<argc; ++i) {
        clusters[i-1] = MSG_get_host_by_name(argv[i]);
    }
    
    speedCoef = getSpeedCoef(clusters, nbClusters);

    jobList = parse("1.wld", "job");
    /*
    {
        job_t job = xbt_malloc(sizeof(*job));
        strcpy(job->name, "job1");
        strcpy(job->service, "Service1");
        job->priority = 0;
        job->weight = 0;
        job->run_time = 100;
        job->wall_time = 150;
        job->nb_procs = 3;
        job->input_size = 0;
        job->output_size = 0;
        jobList = xbt_fifo_new();
        xbt_fifo_push(jobList, job);
        }*/

    /*** MCT ***/ /*
    {
        job_t job;
        while ((job=(job_t)xbt_fifo_shift(jobList)) != NULL) { 
            winner = MCT_schedule(clusters, nbClusters, speedCoef, job);
            
            if (winner->completionT != -1) {
                printf("Winner is %s!\n", winner->cluster->name);
                MSG_task_put(
                    MSG_task_create("SB_TASK", 0, 0, job), winner->cluster, SED_CHANNEL);
            }
            else { printf("no winner!\n"); }
            xbt_free(winner);
        }
    } 
                  */
    
    /*** MinMin or MaxMin ***/ 
    while (xbt_fifo_size(jobList)!=0) {
        winner = MaxMin_schedule(clusters, nbClusters, speedCoef, jobList);
        if (winner->completionT >= 0) {
            printf("Winner is %s!\n", winner->cluster->name);
            MSG_task_put(MSG_task_create("SB_TASK", 0, 0, winner->job), 
                         winner->cluster, SED_CHANNEL);
        }
        else { printf("no winner!\n"); }
        xbt_fifo_remove(jobList, winner->job);
        xbt_free(winner);
    }
                            
    
    /*** HPF ***//*
    {
        while (xbt_fifo_size(jobList)!=0) {
            winner = HPF_schedule(clusters, nbClusters, speedCoef, jobList);
            if (winner->completionT >= 0) {
                printf("Winner is %s!\n", winner->cluster->name);
                MSG_task_put(MSG_task_create("SB_TASK", 0, 0, winner->job), 
                             winner->cluster, SED_CHANNEL);
            }
            else { printf("no winner!\n"); }
            xbt_fifo_remove(jobList, winner->job);
            xbt_free(winner);
        }
        }*/
    
    xbt_fifo_free(jobList);
    xbt_free(speedCoef);
    
    return EXIT_SUCCESS;
}


int sed(int argc, char ** argv) {
    const m_host_t sched = MSG_get_host_by_name(argv[1]);
    const m_host_t batch = MSG_get_host_by_name(argv[2]);
    const int nbServices = argc - 4;
    const char * services[nbServices + 1];
    const double waitT = str2double(argv[3]);
    xbt_fifo_t msg_stack = xbt_fifo_new();
    int i = 0;

    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s - %lf\n", MSG_process_get_name(MSG_process_self()), waitT);
    printArgs(argc, argv);

    services[nbServices] = NULL;
    for (i=0; i<nbServices; ++i) { services[i] = argv[4+i]; }
    for (i=0; services[i]; ++i) { printf("%s\t", services[i]); }
    printf("\n");

    while (1) {
        m_task_t task = NULL;
        MSG_error_t err;

        err = MSG_task_get_with_time_out(&task, SED_CHANNEL, DBL_MAX);
        
        if (err == MSG_TRANSFER_FAILURE) { break; }
        
        if (err == MSG_OK) {
            xbt_fifo_push(msg_stack, task);
            while (MSG_task_Iprobe(SED_CHANNEL)) {
                task = NULL;
                MSG_task_get(&task, SED_CHANNEL);
                xbt_fifo_push(msg_stack, task);
            } 
        }

        while (xbt_fifo_size(msg_stack)) {
            task = xbt_fifo_shift(msg_stack);

            /* Forward to the metaScheduler */ 
            if (MSG_task_get_source(task) == batch) {
                if (!strcmp(task->name, "SB_HPF")) {
                    double * weight = MSG_task_get_data(task);
                    *weight /= nbServices; 
                }
                printf("Forward %s to the MetaScheduler\n", task->name);
                MSG_task_async_put(task, sched, MS_CHANNEL); 
            }
            else {
                job_t job = MSG_task_get_data(task);
                if (isAvailable(services, job->service)) {
                    /* Forward to the Batch */
                    printf("Forward %s to the batch\n", task->name);
                    MSG_task_async_put(task, batch, CLIENT_PORT);
                }
                else { /* Service unavailable */
                    printf("%s Service unavailable %s \n", MSG_host_self()->name, task->name);
                    MSG_task_async_put(MSG_task_create("SB_SERVICE_KO", 0, 0, job), 
                                       sched, MS_CHANNEL); 
                }
            }
        }
    }
    xbt_fifo_free(msg_stack);
    
    return EXIT_SUCCESS;
}


int main(int argc, char ** argv) {
    
    SB_global_init(&argc, argv);
    MSG_global_init(&argc, argv);
    
    /* Open the channels */
    MSG_set_channel_number(NB_CHANNEL);
    
    MSG_function_register("metaSched", metaSched);
    MSG_function_register("sed", sed);
    MSG_function_register("SB_batch", SB_batch);
    MSG_function_register("SB_node", SB_node);

    MSG_create_environment(SB_get_platform_file());
    MSG_launch_application(SB_get_deployment_file()); 

    MSG_main();

    SB_clean();
    MSG_clean();

    return EXIT_SUCCESS;
}
