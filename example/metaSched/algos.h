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


int metaSched(int argc, char ** argv);

int sed(int argc, char ** argv);

double * getSpeedCoef(const m_host_t * cluster, const int nbClusters);

winner_t * MCT_schedule(const m_host_t * clusters, const int nbClusters, 
                        const double * speedCoef, job_t job);

winner_t * MinMin_schedule(const m_host_t * clusters, const int nbClusters, 
                           const double * speedCoef, xbt_fifo_t bagofJobs);

winner_t * MaxMin_schedule(const m_host_t * clusters, const int nbClusters, 
                           const double * speedCoef, xbt_fifo_t bagofJobs);

void HPF(const m_host_t * clusters, const int nbClusters, 
         const double * speedCoef, job_t job,
         const int * nbNodesPF, const int nbNodesTot);

p_winner_t
HPF_schedule(const m_host_t * clusters, const int nbClusters, 
             const double * speedCoef, xbt_fifo_t bagofJobs,
             const int * nbNodesPF, const int nbNodesTot);


#endif
