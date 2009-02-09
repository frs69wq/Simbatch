#ifndef _SED_H_
#define _SED_H_

typedef struct _perf {
  int nb_proc;
  unsigned long int reference_power;
  double time;
} perf;

int sed(int argc, char ** argv);

#endif
