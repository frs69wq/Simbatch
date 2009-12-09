/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <msg/msg.h>

#include "simbatch.h"
#include "simbatch/sb_config.h"

int main(int argc, char ** argv) {
    SB_global_init(&argc, argv);
    MSG_global_init(&argc, argv);
    
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
