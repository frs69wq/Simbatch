#ifndef _DEFS_H_
#define _DEFS_H_

#include <xbt/fifo.h>
#include <msg/msg.h>
#include "simbatch.h"

#define NB_CHANNEL 10000
#define MS_CHAN 42
#define MS_JOB_CHAN 1234
#define MS_CANCEL_CHAN 5678
#define SED_CHAN 10
#define CLIENT_CHAN 666

typedef struct _SGjob {
  char name[15];
  unsigned long int user_id;
  double submit_time;
  double input_size;
  double output_size;
  int nb_procs;
  unsigned int service;
  unsigned int priority;
  double wall_time;
} * SG_job_t;

typedef struct _winner {
    job_t job;
    double completionT;
    m_host_t cluster;
} winner_t, * p_winner_t;

typedef struct _cancelation {
  job_t job;
  m_host_t host;
} cancelation;

#endif
