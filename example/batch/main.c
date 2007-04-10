/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <msg/msg.h>

#include "simbatch.h"
#include "simbatch/sb_config.h"

#define NB_CHANNEL 10000

int main(int argc, char ** argv) {
    const char * trace_file = NULL;
    
    SB_global_init(&argc, argv);
    MSG_global_init(&argc, argv);
 

    /* Open the channels */
    MSG_set_channel_number(NB_CHANNEL);
    
    /* Need a trace */
    trace_file = SB_get_trace_file();
    if (trace_file != NULL)
	MSG_paje_output(trace_file);
    
    /* The client that submits requests (write your own ) 
     * params have to be called with the same name */
    MSG_function_register("SB_client", SB_client);
    /* The batch */
    MSG_function_register("SB_batch", SB_batch);
    /* Node of the Cluster */
    MSG_function_register("SB_node", SB_node);
    
    MSG_create_environment(SB_get_platform_file());
    MSG_launch_application(SB_get_deployment_file());
    
    /* Call MSG_main() */
    MSG_main();
    
    /* Clean everything up */
    SB_clean();
    MSG_clean();

    return EXIT_SUCCESS;
}
