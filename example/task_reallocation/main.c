#include "client.h"
#include "sed.h"
#include "metaSched.h"
#include "simbatch.h"
#include "defs.h"
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char** argv) {
  srand(1);

  FILE * f = fopen("temps.txt", "w");
  fprintf(f, "");
  fclose(f);

  /* initialisation */
  SB_global_init(&argc, argv);
  MSG_global_init(&argc, argv);
    
  /* Open the channels */
  MSG_set_channel_number(NB_CHANNEL);
    
  /* Register functions (scheduler, computation, batch and node) */
  MSG_function_register("client", &client);
  MSG_function_register("metaSched", &metaSched);
  MSG_function_register("resched", &resched);
  MSG_function_register("sed", &sed);
  MSG_function_register("SB_batch", &SB_batch);
  MSG_function_register("SB_node", &SB_node);

  MSG_create_environment(SB_get_platform_file());
  MSG_launch_application(SB_get_deployment_file()); 

  MSG_main();

  SB_clean();
  MSG_clean();

  return EXIT_SUCCESS;
}
