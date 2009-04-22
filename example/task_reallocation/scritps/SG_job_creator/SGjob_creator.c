#include <stdio.h>
#include <stdlib.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

int main (int argc, char** argv) {
  gsl_rng * gBaseRand;
  unsigned long randSeed;
  int i;
  FILE * file;
  int submit_time = 1;
  double wall_time;
  int nb_proc;
  int service;
  int nb_jobs;

  if (argc != 4) {
    printf("usage: %s <jobs_file> <nb_jobs> <seed>", argv[0]);
    return EXIT_FAILURE;
  }

  gBaseRand = gsl_rng_alloc(gsl_rng_default);

  randSeed = atoi(argv[3]);
  srand(randSeed);
  gsl_rng_set (gBaseRand, randSeed);
  nb_jobs = atoi(argv[2]);

  file = fopen(argv[1], "w");
  
  //creation d'un tableau avec tous les services
  int services[nb_jobs];
  int procs[nb_jobs];
  for (i = 0; i < nb_jobs; i++) {
    if (i < nb_jobs * 40 / 100) {
      services[i] = 0;
      procs[i] = rand() % 5 + 1;
    }
    else if (i < nb_jobs * 75 / 100) {
      services[i] = 1;
      procs[i] = rand() % 12 + 5;
    }
    else {
      services[i] = 2;
      procs[i] = rand() % 16 + 15;
    }
  }

  int a;
  int b;
  int tmp;
  int tmp1;
  //melange du tableau
  for (i = 0; i < nb_jobs; i++) {
    a = rand() % nb_jobs;
    b = rand() % nb_jobs;
    tmp = services[a];
    tmp1 = procs[a];
    services[a] = services[b];
    procs[a] = procs[b];
    services[b] = tmp;
    procs[b] = tmp1;
  }

  for (i = 0; i < nb_jobs; i++) 
    {
      service = services[i];
      nb_proc = procs[i];
      //comment this line to use the chosen nb_proc, but always let 
      //the other lines so that the jobs will always be the sames
      //nb_proc = 0;
      wall_time = ((double)rand() / ((double)RAND_MAX + 1.0)) * 2 + 1;
      fprintf(file, "%d\t%d\t0\t0\t%d\t%d\t0\t%lf\n", i, submit_time, 
	      nb_proc, service, wall_time);
      submit_time += gsl_ran_poisson (gBaseRand, 30);
    }
  fclose(file);
  gsl_rng_free(gBaseRand);
  
  return EXIT_SUCCESS;
}





/*
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char** argv) {
  int i;
  FILE *file;
  int service;
  int submit_time = 1;
  int nb_proc;
  double wall_time;
  struct timeval tv;
  int seed;
  if (argc != 4) {
    printf("usage: %s <jobs_file> <nb_jobs> <seed>", argv[0]);
    return EXIT_FAILURE;
  }
  seed = atoi(argv[3]);
  srand(seed);
  file = fopen(argv[1], "w");
  for (i = 0; i < atoi(argv[2]); i++) 
    {
      submit_time += rand() % 91 + 10;
      service = rand() % 100;
      if (service < 50) {
	service = 0;
	nb_proc = rand() % 5 + 1;
      }
      else if (service < 85) {
	service = 1;
	nb_proc = rand() % 10 + 5;
      }
      else {
	service = 2;
	nb_proc = rand() % 20 + 10;
      }
      wall_time = ((double)rand() / ((double)RAND_MAX + 1.0)) * 2 + 1;
      fprintf(file, "%d\t%d\t0\t0\t%d\t%d\t0\t%lf\n", i, submit_time, 
	      nb_proc, service, wall_time);
    }
  fclose(file);
  return EXIT_SUCCESS;
}
*/
