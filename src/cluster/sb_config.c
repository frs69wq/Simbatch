/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include "simbatch_config.h"
#include "sb_config.h"


inline void SB_global_init(int * argc, char ** argv) {
    simbatch_init(argc, argv);
}


/* Get the platform filename from the config */
inline const char * SB_get_platform_file(void) {
    return config_get_platform_file();
}


/* Get the platform filename from the config */
inline const char * SB_get_deployment_file(void) {
    return config_get_deployment_file();
}


inline const char * SB_get_trace_file(void) {
    return config_get_trace_file();
}


inline void SB_clean(void) {
    simbatch_clean();
}

