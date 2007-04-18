/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project get_name : SimBatch     */
/*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <msg/msg.h>

#include "simbatch.h"

#define NB_CHANNEL 10000

void printArgs(const int argc, char ** argv);
int metaSched(int argc, char ** argv);
int sed(int argc, char ** argv);
int batch(int argc, char ** argv);
int node(int argc, char ** argv);


void printArgs(const int argc, char ** argv) {
    int i=1;
    for (i=1; i<argc; ++i) {
        printf("\t%2d - %s\n", i, argv[i]); 
    }
}


int metaSched(int argc, char ** argv) {
    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);
    return EXIT_SUCCESS;
}

int sed(int argc, char ** argv) {
    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);
    return EXIT_SUCCESS;
}

int batch(int argc, char ** argv) {
    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);
    return EXIT_SUCCESS;
}

int node(int argc, char ** argv) {
    printf("%s - ", MSG_host_get_name(MSG_host_self()));
    printf("%s\n", MSG_process_get_name(MSG_process_self()));
    printArgs(argc, argv);
    return EXIT_SUCCESS;
}
    
int main(int argc, char ** argv) {
    
    SB_global_init(&argc, argv);
    MSG_global_init(&argc, argv);

    /* Open the channels */
    MSG_set_channel_number(NB_CHANNEL);
    
    MSG_function_register("metaSched", metaSched);
    MSG_function_register("sed", sed);
    MSG_function_register("batch", batch);
    MSG_function_register("node", node);
    MSG_function_register("SB_batch", SB_batch);
    MSG_function_register("SB_node", SB_node);

    MSG_create_environment(SB_get_platform_file());
    MSG_launch_application(SB_get_deployment_file()); 

    MSG_main();

    SB_clean();
    MSG_clean();

    return EXIT_SUCCESS;
}
