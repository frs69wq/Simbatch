#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/*
  job duration: between 100 and 3000
  requested time: between 1.1 and 3 time the job duration
  nb_proc: between 1 and 5;
  submit_time: tasks arrive at intervals between 100 and 500
*/
int main (int argc, char** argv) {
  int i;
  FILE *file;
  int duration;
  int walltime;
  int submit_time = 0;
  int nb_proc;
  if (argc != 2) {
    printf("usage: %s <nb_jobs>", argv[0]);
    return EXIT_FAILURE;
  }

  srand(time(NULL));
  file = fopen("load.wld", "w");
  for (i = 0; i < atoi(argv[1]); i++) 
    {
      submit_time += rand() % 401 + 100;
      duration = rand()%2901 + 100;
      walltime = duration * (1.1 + 1.9 * (rand() / ((double)RAND_MAX + 1)));
      nb_proc = rand()%5 + 1;
      fprintf(file, "%d\t%d\t%d\t0\t0\t%d\t%d\t0\t0\n", i, submit_time, 
	     duration, walltime, nb_proc);
      //fprintf(file, "job%d\t", i);
    }
  fclose(file);
  return EXIT_SUCCESS;
}
