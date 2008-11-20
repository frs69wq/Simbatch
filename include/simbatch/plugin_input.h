/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _PLUGIN_INPUT_H_
#define _PLUGIN_INPUT_H_

#include <xbt/fifo.h>

/* File used to include parsers for different input files */

/* API */
typedef struct 
{
  xbt_fifo_t (*create_list) (const char * file, const char * name);
} plugin_input, * plugin_input_t;


/* Loading function */
plugin_input_t init_input(plugin_input_t p);


#endif
