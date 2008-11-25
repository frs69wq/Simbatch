/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _ALGOS_H
#define _ALGOS_H

#include <xbt/fifo.h>

#define NB_CHANNEL 10000
#define MS_CHANNEL 10
#define SED_CHANNEL 42

typedef struct _winner {
    job_t job;
    double completionT;
    m_host_t cluster;
} winner_t, * p_winner_t;



p_winner_t MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job);

#endif
