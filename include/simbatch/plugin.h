/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#ifndef _PLUGIN_H
#define _PLUGIN_H

/***********************
 * This module alow us to launch and stop plugins
 * We use only plugin scheduler
 ***********************/

typedef struct _plugin_t 
{
  void * content;
  void * handle;
  const char * name;
  const char * init_fct;
} pluginInfo, * pluginInfo_t;


/* Return an error code (0 => OK) */
int launch_plugin(void ** handle, void * plugin, 
		  const char * plugin_name, const char * init_fct);

int launch_plugin2(pluginInfo_t plugin);

__inline__ void close_plugin(void * handle);

__inline__ void close_pluginInfo(void * data);

pluginInfo_t  SB_request_plugin(const char * init_function, const size_t size);

#endif
