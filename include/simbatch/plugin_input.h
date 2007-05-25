#ifndef _PLUGIN_INPUT_H_
#define _PLUGIN_INPUT_H_

#include <xbt/fifo.h>

/* API */
typedef struct 
{
  xbt_fifo_t (*create_list) (const char * file, const char * name);
} plugin_input, * plugin_input_t;


/* Loading function */
plugin_input_t init_input(plugin_input_t p);


#endif
