/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _PARSER_H_
#define _PARSER_H_

#include <xbt/fifo.h>

/* File not used in the project */

void print(const int * t, const unsigned char);
xbt_fifo_t create_job_from_swf_file();

#endif
