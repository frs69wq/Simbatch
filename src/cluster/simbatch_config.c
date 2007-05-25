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

/* Headers fir the DOM API */
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <xbt/asserts.h>
#include <xbt/dict.h>

#include "optparse.h" 
#include "simbatch_config.h"

/* DIET integration */
char * DIET_FILE = NULL;
unsigned long int DIET_PARAM[4] = {0, 0, 0, 0};
int DIET_MODE = 0;

/* Nb Batch in the system */
static int nbBatch;

/* Simbatch platform config */
static config_t * config;

/* Dictionnary to shared plugin between batch */
static xbt_dict_t book_of_plugin;

/* Dictionnary to shared log file handlers */
#ifdef LOG
static xbt_dict_t book_of_log;

/*** Private functions ***/ 
static int parseCmdLine(int argc, char * argv[]);
static void close_log_file(void * data);


static int parseCmdLine(int argc, char * argv[]) {
    char * allowed[] = {"-f", "-o", "-tw", "-tp", "-sw", "-sp", NULL};
    char * needed[] = {"-f", NULL};
    char * neededWithDIET[] = {"-o", "-tw", "-tp", NULL};
    char * withParams[] = {"-f", "-o", "-tw", "-tp", "-sw", "-sp", NULL};
    
    if (controlParams(argv, withParams, needed, allowed)) {
#ifdef VERBOSE
	fprintf (stderr, "Usage: %s -f config.xml\n", argv[0]);
#endif
	return -1;
    }
    
    if (isPresent(argv, "-o")) {
	if (controlParams(argv, withParams, neededWithDIET, allowed)) {
#ifdef VERBOSE
	    fprintf (stderr, 
		     "Usage: %s -f config.xml -o diet_file -tw uint -tp uint [-sw uint -tw uint]\n", argv[0]);
#endif
	    return -1;
	} 
        DIET_FILE = getParam(argv, "-o");
	DIET_PARAM[0] = secureStr2ul(getParam(argv, "-tw"));
	DIET_PARAM[1] = secureStr2ul(getParam(argv, "-tp"));
	if (!DIET_PARAM[0] || !DIET_PARAM[1]) {
#ifdef VERBOSE
	    fprintf (stderr, "tw && tp must be an uint > 0\n");
#endif
	    return -1;
	}
	DIET_PARAM[2] = (secureStr2ul(getParam(argv, "-sw")))? : 60;
	DIET_PARAM[3] = (secureStr2ul(getParam(argv, "-sp")))? : DIET_PARAM[1];
	DIET_MODE = 1;
    }

    return 0;
}


static void close_log_file(void * data) {
    FILE * flog = (FILE *)data;
    fclose(flog);
    
#ifdef VERBOSE
    fprintf(stderr, "Log file closed\n");
#endif
}


void config_init_log_file(void) {
    xmlXPathObjectPtr xmlobject = NULL;
    const char * r = "/config/batch";
    
#ifdef VERBOSE
    fprintf(stderr, "Init log files : \n");
#endif
    xmlobject = config_get(r);
    if (xmlobject == NULL) {
#ifdef VERBOSE
	fprintf(stderr, "failed\n");
	fprintf(stderr, "XPathError : %s\n", r);
#endif
	free(config);
	exit(2);
    }
    
    if (xmlobject->type == XPATH_NODESET)
	if (xmlobject->nodesetval != NULL) {
	    int i = 0;
	    
	    fprintf(stderr, "\tnb log file : %d\n", xmlobject->nodesetval->nodeNr);
	    for (i=0; i<xmlobject->nodesetval->nodeNr; ++i) {
		char * logfile = NULL;
		FILE * flog = NULL;
		xmlChar * batchName = xmlGetProp(xmlobject->nodesetval->nodeTab[i], 
						 BAD_CAST("host"));
		
		logfile = calloc(xmlStrlen(batchName) + 5, sizeof(char));
		sprintf(logfile, "%s.log", batchName);
		flog = (FILE *)xbt_dict_get_or_null(book_of_log, 
						    (char *)batchName);
		if (flog == NULL) {
		    flog = fopen(logfile, "w");
		    xbt_dict_set(book_of_log, (char *)batchName, flog, 
				 close_log_file);
#ifdef VERBOSE
		    fprintf(stderr, "\tlog file : %s\n", logfile);
#endif
		    free(logfile);
		}
	    }
	}
}


FILE * config_get_log_file(const char * hostname) {
    FILE * floc = (FILE *)xbt_dict_get_or_null(book_of_log, hostname);
    
    return (floc != NULL)? floc: stderr;
}
#endif



config_t * config_load(const char * config_file) {
    config_t * c;
    
    c = malloc(sizeof(*c));
    if (c == NULL)
        return NULL;
    
    /* Analyse du fichier */
    c->doc = xmlParseFile(config_file);
    if (c->doc == NULL) {
        free(c);
        return NULL;
    }
    
    /* Recherche du noeud racine */
    c->root = xmlDocGetRootElement(c->doc);
    
    xmlXPathInit();
    c->context = xmlXPathNewContext(c->doc);
    
    return c;
}


void config_close(void) {
    --nbBatch;
    if (nbBatch == 0) {
#ifdef VERBOSE
        fprintf(stderr, "Config file useless... free\n");
#endif
        free(config), config = NULL;
    }
}


xmlXPathObjectPtr config_get(const char * _xpath) {
    xmlXPathObjectPtr xmlobject = NULL;
    const xmlChar * xpath = BAD_CAST(_xpath);
    
  /* Rquete XPath*/
    xmlobject = xmlXPathEval(xpath, config->context);
    if (xmlobject == NULL)
	return NULL;
    
    if (xmlobject->type == XPATH_NODESET)
	if (xmlobject->nodesetval != NULL)
	    return xmlobject;
    
    xmlXPathFreeObject(xmlobject);
    
    return NULL;
}


const char * config_get_value(const char * _xpath) {
    xmlXPathObjectPtr xmlobject = NULL;
    const xmlChar * xpath = BAD_CAST(_xpath);
    const xmlChar * value = NULL;
    
    
    /* Rquete XPath*/
    xmlobject = xmlXPathEval(xpath, config->context);

    if (xmlobject == NULL)
	return NULL;
    
    if (xmlobject->type == XPATH_NODESET) { 
        if (xmlobject->nodesetval != NULL) { 
            /* nodeNr = nb nodes in struct nodesetval */ 
            if (xmlobject->nodesetval->nodeNr > 0) {
                xmlNodePtr n;
		
                n = xmlobject->nodesetval->nodeTab[0];
                if ((n->type == XML_TEXT_NODE) || (n->type == XML_CDATA_SECTION_NODE))
                    value = n->content;
            }
        }
    }
    
    xmlXPathFreeObject(xmlobject);
    
    return (char *)value;
}


int config_get_nb_nodes(const char * _xpath) {
    xmlXPathObjectPtr xmlobject = NULL;
    const xmlChar * xpath = BAD_CAST(_xpath);
    
    /* Rquete XPath*/
    xmlobject = xmlXPathEval(xpath, config->context);
    if (xmlobject == NULL)
        return -1;
    
    if (xmlobject->type == XPATH_NODESET)
        if (xmlobject->nodesetval != NULL)
            return xmlobject->nodesetval->nodeNr;
    
    xmlXPathFreeObject(xmlobject);
    
    return -1;
}


inline const char * config_get_platform_file(void) {
    const char * platform_file = config_get_value(
        "/config/global/file[@type=\"platform\"]/text()");
    
#ifdef VERBOSE
    fprintf(stderr, "Platform file : ");
#endif
    if (platform_file == NULL) {
#ifdef VERBOSE
        fprintf(stderr, "failed\n");
        fprintf(stderr, 
		"XPathError : /config/global/file[@type=\"platform\"]/text()");
#endif
        
        free(config);
        exit(2);
    }
    else {
#ifdef VERBOSE
        fprintf(stderr, "%s\n", platform_file);
#endif
        return platform_file;
    }    
}


inline const char * config_get_deployment_file(void) {
    const char * deployment_file;
    
#ifdef VERBOSE
    fprintf(stderr, "Deployment file : ");
#endif
    deployment_file = 
	config_get_value("/config/global/file[@type=\"deployment\"]/text()");
    if (deployment_file == NULL) {
#ifdef VERBOSE
	fprintf(stderr, "failed\n");
	fprintf(stderr, 
		"XPathError: /config/global/file[@type=\"deployment\"]/text()");
#endif
	free(config);
	exit(2);
    }
    else {
#ifdef VERBOSE
	fprintf(stderr, "%s\n", deployment_file);
#endif
	return deployment_file;
    }    
}


inline const char * config_get_trace_file(void) {
    const char * trace_file;
    
    trace_file = config_get_value(
	"/config/global/file[@type=\"trace\"]/text()");
    
#ifdef VERBOSE
    if (trace_file == NULL)
	fprintf(stderr, "Trace file : None\n");
    else
	fprintf(stderr, "Trace file : %s\n", trace_file);
#endif
    
    return trace_file;
}


inline void * config_get_plugin(const char * plugin_name) {
    return xbt_dict_get_or_null(book_of_plugin, plugin_name);
}


inline void config_set_plugin(const char * plugin_name, void * plugin, 
			      void * close_plugin) {
    xbt_dict_set(book_of_plugin, plugin_name, plugin, close_plugin);
}


void simbatch_init(int * argc, char ** argv) {
    const char * config_file;    
    const char * deployment_file;
    int nbBatchDeployed = 0;
    
    if (parseCmdLine(*argc, argv)) {
	xbt_die("Error parsing command line");
    }
    config_file = getParam(argv, "-f");
    
#ifdef VERBOSE
    {
	int i = 0;
	fprintf(stderr, "*** Global init ***\n");
	fprintf(stderr, "DIET MODE %s\n", (DIET_MODE)? "enabled": "disable");
	fprintf(stderr, "DIET FILE %s\n", (DIET_FILE)? DIET_FILE: "disable");
	for (i=0; i<4; ++i)
	    fprintf(stderr, "DIET_PARAM[%d] = %lu\n", i, DIET_PARAM[i]);
	fprintf(stderr, "Loading config file %s... ", config_file);
    }
#endif
	
    config = config_load(config_file);
    if (config == NULL) {
#ifdef VERBOSE
	fprintf(stderr, "failed\n");
	fprintf(stderr, "Usage : %s -f simbatch_config.xml\n", argv[0]);
#endif
	free(config);
	xbt_die("Cant'load config file");
    }
    
#ifdef VERBOSE
    fprintf(stderr, "ok\n");
    fprintf(stderr, "Check batch deployed and batch defined... ");
#endif
    
    nbBatch = config_get_nb_nodes("/config/batch");
    deployment_file = config_get_value(
	"/config/global/file[@type=\"deployment\"]/text()");
    
    /* 
     * A bit dirty - I need just one value in the deployment file 
     * So i do a context switch instead of changing my functions
     */
    {
	config_t * config_backup = config;
	
	config = config_load(deployment_file);
	if (config == NULL) {
#ifdef VERBOSE 
	    fprintf(stderr, "failed\n");
#endif
	    free(config_backup);
	    xbt_die("Cant'load deployment file");
	}
	
	nbBatchDeployed = config_get_nb_nodes(
	    "/platform_description/process[@function=\"SB_batch\"]");
	free(config);
	config = config_backup;
    }
    
    if (nbBatchDeployed != nbBatch) {
#ifdef VERBOSE
        fprintf(stderr, "failed\n");
#endif
        free(config);
        xbt_die("Batch deployed are not equal to batch defined");
    }
    
#ifdef VERBOSE
    fprintf(stderr, "ok\n");
    
    if (nbBatch <= 0)
        fprintf(stderr, "Warning no batch used\n");
    
    fprintf(stderr, "Number of batch defined  : %d\n", nbBatch);
#endif
    
    book_of_plugin = xbt_dict_new(); 
    
#ifdef LOG
    book_of_log = xbt_dict_new();
    config_init_log_file();
#endif
    
}


inline void simbatch_clean(void) {
    if (config != NULL)
        free(config);
    
#ifdef LOG
    xbt_dict_free(&book_of_log);
#endif 
    xbt_dict_free(&book_of_plugin);
}
