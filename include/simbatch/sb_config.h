/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _SB_CONFIG_H_
#define _SB_CONFIG_H_

/* This file just defines wrapping functions for those
 * defined in simbatch config. It allows the client not
 * to have to defined libxml2 cflags and libs to declare
 * in its makefile  
 */

/* Call to simbatch init */
__inline__ void SB_global_init(int * argc, char ** argv);

/* Call to simbatch clean */
__inline__ void SB_clean(void);

/* Get the name of the platform file from the config */
__inline__ const char * SB_get_platform_file(void);

/* Get the name of the platform file from the config */
__inline__ const char * SB_get_deployment_file(void);

/* Get the name of the trace file from the config */
__inline__ const char * SB_get_trace_file(void);

#endif
