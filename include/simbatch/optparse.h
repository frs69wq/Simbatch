#ifndef _OPTPARSE_H
#define _OPTPARSE_H

/* Macros de traitement des tableaux null-terminated. */
#define ARRAYSIZE(a) ({int _sz__=0; while ((a)[_sz__++]); --_sz__;})
#define FOREACH(a,fun) ({int _i__=0; while ((a)[_i__]) fun((a)[_i__++]);})
#define ARRAYEND(a) ({int _i__=0; while ((a)[_i__]) _i__++; &((a)[_i__]);})

/* Utils ********************************************/
void shiftL(char * argv[]);
int indexOf(char * argv[], const char * arg);
int belongs(char * argv[], const char * s);
int hasParam(char * argv[], int index, char * allargs[]);
int hasArg(char * argv[], int index, char * allparams[]);
inline int isPresent(char * argv[], const char * arg);
char * getParam(char * argv[], const char * arg);
int controlParams(char * argv[], char * allparams[], char * needed[], char * allargs[]);
int controlLeavingParams(char * argv[], char * allparams[], char * allargs[]);
unsigned long int secureStr2ul(char * arg);

#endif
