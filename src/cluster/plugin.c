/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "simbatch_config.h"
#include "utils.h"
#include "plugin.h"


int
launch_plugin(void **handle, void *plugin, const char *plugin_name,
              const char *init_fct)
{
    void * (*init) (void *p);
    
    *handle = dlopen(plugin_name, RTLD_LAZY);
    if (!*handle) {
        fprintf(stderr, "dlopen %s\n", dlerror());
        return 1;
    }
    
    init = dlsym(*handle, init_fct);
    if (!init) {
        fprintf(stderr, "dlsym %s\n", dlerror());
        dlclose(*handle);
        return 1;
    }
    
    if (!init(plugin)) {
        fprintf(stderr, "init %s\n", dlerror());
        dlclose(*handle);
        return 1;
    }
    
    return 0;
}


int
launch_plugin2(pluginInfo_t plugin)
{
    void * (*init) (void *p);
    
    plugin->handle = dlopen(plugin->name, RTLD_LAZY);
    if (plugin->handle == NULL) {
        fprintf(stderr, "dlopen %s\n", dlerror());
        return 1;
    }
    
    init = dlsym(plugin->handle, plugin->init_fct);
    if (!init) {
        fprintf(stderr, "dlsym %s\n", dlerror());
        dlclose(plugin->handle);
        return 1;
    }
    
    if (!init(plugin->content)) {
        fprintf(stderr, "init %s\n", dlerror());
        dlclose(plugin->handle);
        return 1;
    }
    
    return 0;
}


inline void
close_plugin(void *handle)
{
    dlclose(handle);
}


inline void
close_pluginInfo(void *data)
{
    pluginInfo_t p = (pluginInfo_t)data;
    
    dlclose(p->handle);

#ifdef VERBOSE 
    fprintf(stderr, "Close plugin %s... ok\n", p->name);
#endif
    
    free(p->content);
    free(p);  
    
#ifdef VERBOSE
    fprintf(stderr, "Free memory... ok\n");
#endif
    
}


pluginInfo_t
SB_request_plugin(const char *init_function, const size_t size)
{
  pluginInfo_t plugin = NULL;
  char request[128];
  const char *plugin_name;

  /*** Plug in ***/
#ifdef VERBOSE
  fprintf(stderr, "Plugin init...\t");
#endif
  
  sprintf(request, "/config/batch[@host=\"%s\"]/plugin/text()",\
	  MSG_host_get_name(MSG_host_self()));
  
  plugin_name = config_get_value(request);
  if (plugin_name == NULL) {
#ifdef VERBOSE
      fprintf(stderr, "failed\n");
      fprintf(stderr, "XPathError : %s\n", request);
#endif 
      return NULL;
  }
  
  /* search if the plugin have already been load */
  plugin = (pluginInfo_t)config_get_plugin(plugin_name); 
  if (plugin == NULL) {
      plugin = calloc(1, sizeof(*plugin));
      plugin->content = calloc(1, size);
      plugin->init_fct = init_function;
      plugin->name = plugin_name;
      if (launch_plugin2(plugin) != 0)
	{
#ifdef VERBOSE
	  fprintf(stderr, "failed\n");
	  fprintf(stderr, "Call to launch_plugin failed\n");
#endif
	  return NULL;
	}
      else
	config_set_plugin(plugin_name, plugin, close_pluginInfo);
    }
  
#ifdef VERBOSE
    fprintf(stderr, "ok\n");
#endif

    return plugin;
}
