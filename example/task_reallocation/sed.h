#ifndef _SED_H_
#define _SED_H_

typedef struct _perf {
  int nb_proc;
  unsigned long int reference_power;
  double time;
  unsigned int service;
} perf;

typedef struct _performances {
  perf ** p;
  int * sizes;
  int nbP;
} performances;

int sed(int argc, char ** argv);

#endif
