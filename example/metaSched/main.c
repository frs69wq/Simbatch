/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project get_name : SimBatch     */
/*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <xbt/fifo.h>
#include <xbt/sysdep.h>
#include <msg/msg.h>

#include "simbatch.h"

#define NB_CHANNEL 10000
#define MS_CHANNEL 10
#define SED_CHANNEL 42


typedef struct _winner {
    job_t job;
    double completionT;
    m_host_t cluster;
} winner_t, * p_winner_t;


void printArgs(int argc, char **argv);

int metaSched(int argc, char ** argv);

int sed(int argc, char ** argv);

double * getSpeedCoef(m_host_t * cluster, int nbClusters);

winner_t * MCT_schedule(m_host_t * clusters, int nbClusters, 
                        double * speedCoef, job_t job);

winner_t * MinMin_schedule(m_host_t * clusters, int nbClusters, 
                           double * speedCoef, xbt_fifo_t bagofJobs);

winner_t * MaxMin_schedule(m_host_t * clusters, int nbClusters, 
                           double * speedCoef, xbt_fifo_t bagofJobs);



void printArgs(int argc, char ** argv) {
    int i=1;
    for (i=1; i<argc; ++i) {
        printf("\t%2d - %s\n", i, argv[i]); 
    }
}


/* Ratio between clusters */
double * getSpeedCoef(m_host_t * clusters, int nbClusters) {
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
        printf("\t\t\t%lf\n", speedCoef[i]);
    }

    return speedCoef;
}


winner_t * MCT_schedule(m_host_t * clusters, int nbClusters, double * speedCoef, job_t job) {
    int i = 0;
    double min = DBL_MAX;
    winner_t * winner = xbt_malloc(sizeof(winner_t));
    
    // broadcast 
    for (i=0; i<nbClusters; ++i) {    
        MSG_task_put(MSG_task_create("SED_PRED", 0, 0, job), clusters[i], SED_CHANNEL);
    }
    
    // get
    for (i=0; i<nbClusters; ++i) {
        m_task_t task = NULL;
        slot_t * slots = NULL;
        
        MSG_task_get(&task, MS_CHANNEL);
        slots = MSG_task_get_data(task);
        MSG_task_destroy(task);
        task = NULL;
        print_slot(slots, 5);
        printf("\n");
        winner->completionT = slots[0]->start_time + (job->wall_time * speedCoef[i]);
        if (winner->completionT  < min) {
            min = winner->completionT;
            winner->cluster = clusters[i];
            winner->job = job; 
        }
        xbt_free(slots);
    }
    return winner;
}


p_winner_t
MinMin_schedule(m_host_t * clusters, const int nbClusters, 
                double * speedCoef, xbt_fifo_t bagofJobs) {
    p_winner_t winners[xbt_fifo_size(bagofJobs)];
    p_winner_t bigWinner = NULL;
    xbt_fifo_item_t bucket = NULL;
    job_t job = NULL;
    int i = 0;
    
    // foreach job select min MCT
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) {
        winners[i++] = MCT_schedule(clusters, nbClusters, speedCoef, job);
    }
    
    // foreach MCT estimation, select the min
    bigWinner = winners[0];
    for (i=1; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i]->completionT < bigWinner->completionT) {
            bigWinner = winners[i];
        }
    }
    
    return bigWinner;
}


p_winner_t
MaxMin_schedule(m_host_t * clusters, const int nbClusters, 
                double * speedCoef, xbt_fifo_t bagofJobs) {
    p_winner_t winners[xbt_fifo_size(bagofJobs)];
    p_winner_t bigWinner = NULL;
    xbt_fifo_item_t bucket = NULL;
    job_t job = NULL;
    int i = 0;
    
    // foreach job select min MCT
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) {
        winners[i++] = MCT_schedule(clusters, nbClusters, speedCoef, job);
    }
    
    // foreach MCT estimation, select the min
    bigWinner = winners[0];
    for (i=1; i<xbt_fifo_size(bagofJobs); ++i) {
        if (winners[i]->completionT > bigWinner->completionT) {
            bigWinner = winners[i];
        }
    }
    
    return bigWinner;
}



int metaSched(int argc, char ** argv) {
    m_host_t * clusters = NULL;
    xbt_fifo_t jobList = NULL;
    winner_t * winner = NULL;
    double * speedCoef = NULL;
    static int cpt = 0;
    int nbClusters = argc - 1;
    int i = 1;

    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);

    clusters = xbt_malloc(sizeof(m_host_t) * nbClusters);
    for (i=1; i<argc; ++i) {
        clusters[i-1] = MSG_get_host_by_name(argv[i]);
        printf("t %s\n", clusters[i-1]->name);
    }
    
    speedCoef = getSpeedCoef(clusters, nbClusters);
    
    jobList = xbt_fifo_new();
    {
        job_t job =  NULL; 
        job =  xbt_malloc(sizeof(*job));
        
        // Warning: should the job be copied or not?
        strcpy(job->name, "SED_PRED");
        job->submit_time = MSG_get_clock();
        job->input_size = 0.0; 
        job->output_size = 0.0;
        job->priority = 0;
        job->nb_procs = 3;
        job->wall_time = 150;
        job->run_time = 105;
        job->state = WAITING;
        xbt_fifo_push(jobList, job);
    }
    
    {
        job_t job = NULL;
        job = xbt_malloc(sizeof(*job)); // freed by SB_batch
        strcpy(job->name, "SED_PRED");
        job->submit_time = MSG_get_clock();
        job->input_size = 0.0; 
        job->output_size = 0.0;
        job->priority = 0;
        job->nb_procs = 3;
        job->wall_time = 200;
        job->run_time = 100;
        job->state = WAITING;
        xbt_fifo_push(jobList, job);
    }
    /***** MCT  
    {
        job_t job;
        while ((job=(job_t)xbt_fifo_shift(jobList)) != NULL) { 
            winner = MCT_schedule(clusters, nbClusters, speedCoef, job);
            
            printf("Winner is %s!\n", winner->cluster->name);
            sprintf(job->name, "job%d", cpt);
            MSG_task_put(
                MSG_task_create("SB_TASK", 0, 0, job), winner->cluster, SED_CHANNEL);
            xbt_free(winner);
        }
    }
    */
    
    while (xbt_fifo_size(jobList)!=0) {
        winner = MaxMin_schedule(clusters, nbClusters, speedCoef, jobList);
        printf("Winner is %s!\n", winner->cluster->name);

        sprintf(winner->job->name, "job%d", cpt++);
        MSG_task_put(MSG_task_create("SB_TASK", 0, 0, winner->job), 
                     winner->cluster, SED_CHANNEL);
        xbt_fifo_remove(jobList, winner->job);
        xbt_free(winner);
    }
    
    xbt_fifo_free(jobList);
    xbt_free(clusters);
    xbt_free(speedCoef);
    
    return EXIT_SUCCESS;
}


int sed(int argc, char ** argv) {
    m_host_t cluster = NULL;
    m_host_t sched = NULL;
    xbt_fifo_t msg_stack = xbt_fifo_new();
                      
    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);

    /* init */
    sched = MSG_get_host_by_name(argv[1]);
    cluster = MSG_get_host_by_name(argv[2]);

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
            
            if (!strcmp(task->name, "SB_PRED")) {
                /* Forward to the MS */
                printf("Forward %s to the Metascheduler\n", task->name);
                MSG_task_async_put(task, sched, MS_CHANNEL); 
            }
            else {
                /* Forward to the Batch */
                printf("Forward %s to the batch\n", task->name);
                MSG_task_async_put(task, MSG_host_self(), CLIENT_PORT);
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
