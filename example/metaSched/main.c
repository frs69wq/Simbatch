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

void printArgs(int argc, char **argv);
int metaSched(int argc, char ** argv);
int sed(int argc, char ** argv);
m_host_t MCT_schedule(int nbClusters, m_host_t * clusters, job_t job);

void printArgs(int argc, char ** argv) {
    int i=1;
    for (i=1; i<argc; ++i) {
        printf("\t%2d - %s\n", i, argv[i]); 
    }
}


m_host_t MCT_schedule(int nbClusters, m_host_t * clusters, job_t job) {
    int i = 0;
    double min = DBL_MAX;
    m_host_t winner = NULL;

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
        if (slots[0]->start_time < min) {
            min = slots[0]->start_time;
            winner = clusters[i];
        }
        xbt_free(slots);
    }
    return winner;
}


int metaSched(int argc, char ** argv) {
    m_host_t * clusters = NULL;
    m_host_t winner = NULL;
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
        job->run_time = 100;
        job->state = WAITING;
  
        winner = MCT_schedule(nbClusters, clusters, job);
        
        printf("Winner is %s!\n", winner->name);
        strcpy(job->name, "job");
        MSG_task_put(MSG_task_create("SB_TASK", 0, 0, job), winner, SED_CHANNEL);
    }
    
    MSG_process_sleep(10);
    
    {
        job_t job =  xbt_malloc(sizeof(*job)); // freed by SB_batch
        strcpy(job->name, "SED_PRED");
        job->submit_time = MSG_get_clock();
        job->input_size = 0.0; 
        job->output_size = 0.0;
        job->priority = 0;
        job->nb_procs = 3;
        job->wall_time = 150;
        job->run_time = 100;
        job->state = WAITING;
        
        winner = MCT_schedule(nbClusters, clusters, job);
        
        printf("Winner is %s!\n", winner->name);
        strcpy(job->name, "job1"); 
        MSG_task_put(MSG_task_create("SB_TASK", 0, 0, job), winner, SED_CHANNEL);
    }
    
    xbt_free(clusters);
    
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
