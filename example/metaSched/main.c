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

#define NB_CHANNEL 10000
#define MS_CHANNEL 10
#define SED_CHANNEL 42


typedef struct _winner {
    job_t job;
    double completionT;
    m_host_t cluster;
} winner_t, * p_winner_t;


void printArgs(int argc, char **argv);

int isAvailable(const char * services[], const char * service);

double str2double(const char * str);

int metaSched(int argc, char ** argv);

int sed(int argc, char ** argv);

double * getSpeedCoef(const m_host_t * cluster, const int nbClusters);

winner_t * MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job);

winner_t * MinMin_schedule(const m_host_t * clusters, const int nbClusters, 
                           const double * speedCoef, xbt_fifo_t bagofJobs);

winner_t * MaxMin_schedule(const m_host_t * clusters, const int nbClusters, 
                           const double * speedCoef, xbt_fifo_t bagofJobs);

double HPF(const m_host_t * clusters, const int nbClusters, 
           const double * speedCoef, job_t job);



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


winner_t * MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job) {
    int i = 0;
    double completionT = DBL_MAX;
    winner_t * winner = xbt_malloc(sizeof(winner_t));
    
    winner->completionT = DBL_MAX;
    winner->job = job;
    
    // broadcast 
    for (i=0; i<nbClusters; ++i) {    
        MSG_task_put(MSG_task_create("SED_PRED", 0, 0, job), clusters[i], SED_CHANNEL);
    }
    
    // get
    for (i=0; i<nbClusters; ++i) {
        m_task_t task = NULL;
        
        MSG_task_get(&task, MS_CHANNEL);
        if (!strcmp(task->name, "SB_PRED")) { // OK
            slot_t * slots = NULL;
            slots = MSG_task_get_data(task);
            print_slot(slots, 5);
            completionT = slots[0]->start_time + (job->wall_time * speedCoef[i]);
            if (completionT < winner->completionT) { 
                winner->completionT = completionT;
                winner->cluster = slots[0]->host;
            }
            xbt_free(slots);
        }
        else { 
            const m_host_t sender = MSG_task_get_source(task);
            printf("Service unavailable on host: %s\n", sender->name);
        }
        
        printf("\n");
        MSG_task_destroy(task), task = NULL;
    }
    return winner;
}


p_winner_t
MinMin_schedule(const m_host_t * clusters, const int nbClusters, 
                const double * speedCoef, xbt_fifo_t bagofJobs) {
    p_winner_t winners[xbt_fifo_size(bagofJobs)];
    p_winner_t bigWinner = NULL;
    xbt_fifo_item_t bucket = NULL;
    job_t job = NULL;
    int i = 0;
    
    // foreach job select min MCT
    xbt_fifo_foreach(bagofJobs, bucket, job, typeof(job)) {
        winners[i++] = MCT_schedule(clusters, nbClusters, speedCoef, job);
        printf("qdfdsf %lf\n", winners[i-1]->completionT); 
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
MaxMin_schedule(const m_host_t * clusters, const int nbClusters, 
                const double * speedCoef, xbt_fifo_t bagofJobs) {
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


double HPF(const m_host_t * clusters, const int nbClusters, 
           const double * speedCoef, job_t job) {
    double weight = 0;
    int i = 0;
    
    // broadcast 
    for (i=0; i<nbClusters; ++i) {    
        MSG_task_put(MSG_task_create("SED_HPF", 0, 0, job), clusters[i], SED_CHANNEL);
    }
    
     // get
    for (i=0; i<nbClusters; ++i) {
        m_task_t task = NULL;
        
        MSG_task_get(&task, MS_CHANNEL);
        weight = *((double *) MSG_task_get_data(task));
        MSG_task_destroy(task);
        task = NULL;
       
        xbt_free(&weight);
    }
    
    return weight;
}


int metaSched(int argc, char ** argv) {
    static int cpt = 0;
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
    
    jobList = xbt_fifo_new();
    {
        job_t job =  xbt_malloc(sizeof(*job));
        
        // Warning: should the job be copied or not?
        strcpy(job->name, "SED_PRED");
        strcpy(job->service, "Service1");
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
        job_t job = xbt_malloc(sizeof(*job)); // freed by SB_batch
        
        strcpy(job->name, "SED_PRED");
        strcpy(job->service, "Service2");
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
    /*** MCT ***/ 
    {
        job_t job;
        while ((job=(job_t)xbt_fifo_shift(jobList)) != NULL) { 
            winner = MCT_schedule(clusters, nbClusters, speedCoef, job);
            
            printf("Winner is %s!\n", winner->cluster->name);
            sprintf(job->name, "job%d", ++cpt);
            MSG_task_put(
                MSG_task_create("SB_TASK", 0, 0, job), winner->cluster, SED_CHANNEL);
            xbt_free(winner);
        }
    }
  
    
    /*** MinMin or MinMax ***/ /*
    while (xbt_fifo_size(jobList)!=0) {
        winner = MaxMin_schedule(clusters, nbClusters, speedCoef, jobList);
        printf("Winner is %s!\n", winner->cluster->name);

        sprintf(winner->job->name, "job%d", cpt++);
        MSG_task_put(MSG_task_create("SB_TASK", 0, 0, winner->job), 
                     winner->cluster, SED_CHANNEL);
        xbt_fifo_remove(jobList, winner->job);
        xbt_free(winner);
    }
                               */
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
