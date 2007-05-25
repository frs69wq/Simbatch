#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "optparse.h" 

#define ARRAYSIZE(a) ({int _sz__=0; while ((a)[_sz__++]); --_sz__;})
#define FOREACH(a,fun) ({int _i__=0; while ((a)[_i__]) fun((a)[_i__++]);})
#define ARRAYEND(a) ({int _i__=0; while ((a)[_i__]) _i__++; &((a)[_i__]);})

/* Utils */
void shiftL(char * argv[]) {
    int i=0;
    auto void _shiftL(char * s);
    void _shiftL(char * s) { argv[i]=argv[i+1];i++; }
    
    FOREACH(argv,_shiftL);
}

int indexOf(char * argv[], const char * arg) {
    //__label__ exit;
    int ret=-1, i=0;
    auto void _search(const char * s);
    void _search(const char * s)
    { if (!strcmp(s,arg)) { ret=i; /*goto exit;*/} i++; }
    
    FOREACH(argv,_search);
    //exit :
    return ret;
}

int belongs(char * argv[], const char * s) {
    //__label__ exit;
    int ret=0;
    auto void _equal(const char * arg);
    void _equal(const char * arg) 
    { if(!strcmp(s,arg)) { ret=1; /*goto exit;*/ } }
    
    if (!s) return 0;
    FOREACH(argv,_equal);
    //exit :
    return ret;
}

int hasParam(char * argv[], int index, char * allargs[]) {
    if (index>=ARRAYSIZE(argv)-1 || index<0) return 0;
    return !belongs(allargs, argv[index+1]);  
}

int hasArg(char * argv[], int index, char * allparams[]) {
    if (index<2) return 0;
    return belongs(allparams, argv[index-1]);
}

inline int isPresent(char * argv[], const char * arg) {
    return belongs(argv, arg);
}

char * getParam(char * argv[], const char * arg) {
    int index =indexOf(argv, arg);
    
    if (index==-1) return NULL;
    else return argv[index+1];
}

int controlParams(char * argv[], char * allparams[], char * needed[],
		  char * allargs[]) {
    int ret=0;
    auto void _needed(const char * s);
    auto void _needsparam(const char * s);

    void _needed(const char * s) {
	if (!belongs(argv,s)) {
	    fprintf(stderr,"%s argument is needed.\n",s);
	    ret=-1;
	}
    }
    void _needsparam(const char * s) {
	if (belongs(allparams,s) && !hasParam(argv,indexOf(argv,s),allargs)) {
	    fprintf(stderr, "Argument %s needs a parameter.\n", s);
	    ret=-2;
	}
    }
    
    FOREACH(needed, _needed);
    FOREACH(argv, _needsparam);
    
    return ret;
}

int controlLeavingParams(char * argv[], char * allparams[], char * allargs[]) {
    int ret=0;
    auto void _argorparam(const char * s);
    void _argorparam(const char * s) {
	int ind=indexOf(argv,s);
	if (!belongs(allargs, s) && !hasArg(argv,ind,allparams))
	    if (ind) {
		fprintf(stderr, "WARNING: %s is not a valid option.\n", s);
		ret=-3;
	    }
    }
    FOREACH(argv, _argorparam);
    return ret;
}

unsigned long int secureStr2ul(char * arg) {
    unsigned long int value = 0;
    char * fin;
    
    if (!arg) return 0;
    
    value = strtoul(arg, &fin, 0);
    if (fin == arg) return 0;
    
    if (((value == 0) || (value == ULONG_MAX)) && (errno == ERANGE)) return 0;
    
    return value;
} 
