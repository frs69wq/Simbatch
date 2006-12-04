/* Headers fir the DOM API */
#ifndef _SIMBATCH_CONFIG_H_
#define _SIMBATCH_CONFIG_H_

#include <libxml/tree.h>
#include <libxml/xpath.h>


/* Config structure */
typedef struct
{
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlXPathContextPtr context;
} config_t;


/* Init simbatch library */
void simbatch_init(int * argc, char ** argv);

/* Close handlers and other stuff */
void simbatch_clean(void);

/* Load an xml file - DOM tree */
config_t * config_load(const char * filename);

/* Free the global config */
void config_close(void);

/* Get the whole answer structure */
xmlXPathObjectPtr config_get(const char * _xpath);

/* Get the first value from an XPath request */
const char * config_get_value(const char * _xpath);

/* Get number of nodes matching an XPath request */
int config_get_nb_nodes(const char * _path);

/* Get the name of the platform file from the config */
__inline__ const char * config_get_platform_file(void);

/* Get the name of the deployment file from the config */
__inline__ const char * config_get_deployment_file(void);

/* Get the name of the trace file from the config */
__inline__ const char * config_get_trace_file(void);

/* Init the log file system */
void config_init_log_file(void);

/* Get the descriptor for the logfile associated to an host */
FILE * config_get_log_file(const char * hostname);

/* Return a plugin from the book of plugin - NULL otherwise*/
__inline__ void * config_get_plugin(const char * plugin_name);

/* Set a new plugin in the book of plugin */
__inline__ void config_set_plugin(const char * plugin_name, void * plugin, void * close_plugin);

#endif
