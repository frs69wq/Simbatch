#ifndef _EXTERNAL_LOAD_H_
#define _EXTERNAL_LOAD_H_

/* Request the name of the external load file to the config */ 
const char * SB_request_external_load(void);

const char * SB_request_parser(void);

/* Process that handle the external load */
int SB_external_load(int argc, char ** argv);

#endif
