#include <stdio.h>
#include <stdlib.h>

int main (int argc, char * argv[]) {
  
  double parallelization;
  FILE * output_file;
  int min_proc;
  int max_proc;
  double time;
  int i;
  double exec_time;

  if (argc != 6) {
    printf("Usage: %s <output_file> <min_proc> <max_proc> "
	   "<parallelization> <time_one_processor>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  
  output_file = fopen(argv[1], "w");
  min_proc = atoi(argv[2]);
  max_proc = atoi(argv[3]);
  parallelization = atof(argv[4]);
  time = atof(argv[5]);

  for (i = 1; i <= max_proc; i++) {
    exec_time = time / (1/((1-parallelization) + parallelization / i));
    if (i >= min_proc) {
      fprintf(output_file, "%d %lf\n", i, exec_time);
    }
  }

  fclose(output_file);
  return EXIT_SUCCESS;
}
