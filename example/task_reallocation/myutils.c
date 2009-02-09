#include <stdio.h>
#include "myutils.h"

void printArgs(int argc, char ** argv) {
  int i = 0;
  printf("%s\n", argv[i]); 
  for (i=1; i<argc; ++i) {
    printf("\t%2d - %s\n", i, argv[i]); 
  }
}

