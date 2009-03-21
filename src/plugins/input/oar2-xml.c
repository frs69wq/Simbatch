/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jonathan Amiez, ENS Lyon                                      */
/*                                                                          */
/* Copyright (c) 2009 Simbatch Team. All rights reserved.                   */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>

#include "plugin_input.h"
#include "job.h"

/** kinds of elements we use from OAR xml files **/
enum {
    OTHER,
    JOB_ID,
    SUB_TIME,
    RESOURCES,
    WALL_TIME_1,
    WALL_TIME_2
};

/********** Global vars ***********/
/** takes values from the above enum **/
static int current_elt;

/** counts number of core the job uses **/
static int counter;

/** how deep we are in the xml hierarchy **/
static int depth;

/** take the 'walltime' node value or the 'wanted_resources' node one **/
static int walltime; //boolean

/** count assigned resources or reserved resources **/
static int assigned; //boolean

/** Array where useful infos are stored before be added to job_t **/
static char* infos[4];

/** job's list **/
static xbt_fifo_t list;


/********** Functions ***********/
/** Called when the parser reaches the start of document **/
static void start_doc(void* user_data);

/** Called when the parser reaches the end of document **/
static void end_doc(void* user_data);

/** Called when the parser reaches the start of an element **/
static void start_elt(void* user_data, const xmlChar* name,
                      const xmlChar** attrs);

/** Called when the parser reaches the end of an element **/
static void end_elt(void* user_data, const xmlChar* name);

/** Called when the parser reaches the content of an element **/
static void chars(void* user_data, const xmlChar* ch, int len);

/** Main function **/
static xbt_fifo_t oar2_xml_parse(const char* file, const char* name);


plugin_input_t init_input(plugin_input_t p) {
    p->create_list = oar2_xml_parse;
    return p;
}


static void start_doc(void* user_data) {
    int i;

    fprintf(stderr, "Parsing XML file...\n");
    current_elt = OTHER;
    counter = 0;
    depth = 0;
    walltime = 1; //true
    assigned = 1; //true
    for (i=0; i<4; i++) {
        infos[i] = calloc(15, sizeof(char));
    }
}

static void end_doc(void* user_data) {
    int i;
    
    for (i=0; i<4; i++) {
        free(infos[i]);
    }
    fprintf(stderr, "Done.\n");
}

static void start_elt(void* user_data, const xmlChar* name, const xmlChar** attrs) {
    ++depth;
    /** process job id node **/
    if ( !xmlStrcmp( name, (xmlChar*)"item" ) &&
         !xmlStrcmp( attrs[1], (xmlChar*)"Job_Id" ) ) {
        current_elt = JOB_ID;
    }
    /** process resources node **/
    else if ( !xmlStrcmp( name, (xmlChar*)"item" ) && 
              !xmlStrcmp( attrs[1], (xmlChar*)"assigned_resources" ) &&
              assigned ) {
        current_elt = RESOURCES;
    }
    else if ( !xmlStrcmp( name, (xmlChar*)"item" ) &&
              !xmlStrcmp( attrs[1], (xmlChar*)"reserved_resources" ) &&
              !assigned ) {
        current_elt = RESOURCES;
    }
    else if ( current_elt == RESOURCES && depth == 7 ) {
        ++counter;
    }
    /** process submission time node **/
    else if ( !xmlStrcmp( name, (xmlChar*)"item" ) &&
              !xmlStrcmp( attrs[1], (xmlChar*)"submissionTime" ) ) {
        current_elt = SUB_TIME;
    }
    /** process walltime node **/
    else if ( !xmlStrcmp( name, (xmlChar*)"item" ) &&
              !xmlStrcmp( attrs[1], (xmlChar*)"walltime" ) ) {
        if ( !xmlStrcmp( attrs[2], (xmlChar*)"defined" ) )
            walltime = 0;
        else
            current_elt = WALL_TIME_1;
    }
    else if ( !xmlStrcmp( name, (xmlChar*)"item" ) &&
              !xmlStrcmp( attrs[1], (xmlChar*)"wanted_resources" ) &&
              !walltime ) {
        current_elt = WALL_TIME_2;
    }
}

static void end_elt(void *user_data, const xmlChar *name) {
    job_t job;

    --depth;
    if ( current_elt == RESOURCES && depth == 5 ) {
        if( counter == 0 )
            assigned = 0;
        else
            sprintf(infos[3], "%d", counter);
        counter = 0;
        current_elt = OTHER;
    }
    else if ( current_elt == RESOURCES && depth > 5 ) {
        current_elt = RESOURCES;
    }
    else if ( depth == 3 ) {
        /** Hard coded values **/
        job = xbt_malloc(sizeof(*job));
        job->start_time = 0.0;
        job->state = WAITING;
        job->free_on_completion = 1;
        job->input_size = 0.0;
        job->output_size = 0.0;
        job->priority = 0;
        job->service = 0;
        
        /** Parsed values from config file **/
        sprintf(job->name, "%s%s", name, infos[0]);
        job->user_id = atol(infos[0]);
        job->submit_time = atol(infos[1]);
        job->run_time = atol(infos[2]);
        job->wall_time = atol(infos[2]);
        job->nb_procs = atoi(infos[3]);
#ifdef DEBUG
        fprintf(stderr, "%s %lu %lf %lf %lf %d\n",
                job->name, job->user_id, job->submit_time,
                job->run_time, job->wall_time, job->nb_procs);
#endif
	    xbt_fifo_push(list,job);

        // Re-init of boolean variables
        walltime = 1;
        assigned = 1;

        current_elt = OTHER;
    }
    else
        current_elt = OTHER;
}

static void chars(void *user_data, const xmlChar *ch, int len) {
    char* str = NULL;
    char* ctmp;
    unsigned long itmp = 0;

    if( current_elt == JOB_ID ) {
        sprintf(infos[0], "%.*s", len, ch);
    }
    else if( current_elt == SUB_TIME ) {
        sprintf(infos[1], "%.*s", len, ch);
    }
    else if( current_elt == WALL_TIME_1 ) {
        sprintf(infos[2], "%.*s", len, ch);
    }
    else if( current_elt == WALL_TIME_2 ) {
        str = calloc(15, sizeof(char));
        if( str == NULL ) {
            fprintf(stderr, "Malloc error\n");
            exit( EXIT_FAILURE );
        }
        sprintf(str, "%.*s\0", len, ch);
        ctmp = strtok(str, ":");
        itmp = atol(ctmp) * 3600;
        ctmp = strtok(NULL, ":");
        itmp = itmp + (atol(ctmp) * 60);
        ctmp = strtok(NULL, ":");
        itmp = itmp + atol(ctmp);
        sprintf(infos[2], "%ld", itmp);
        free(str);
    }
}

static xbt_fifo_t oar2_xml_parse(const char* file, const char* name) {
    char* tmp = NULL;
    const char* sed_cmd = "sed -i '/wanted_resources/s/-l.*,.*=//g; \
                           /wanted_resources/s/&quot; //g'";
    xmlSAXHandler sh = { 0 };

    tmp = calloc(strlen(sed_cmd) + strlen(file) + 2, sizeof(char));
    sprintf(tmp, "%s %s", sed_cmd, file);
    if (system(tmp) != 0) {
        fprintf(stderr, "Error during editing XML file (sed).\n");
        return NULL;
    }
    free(tmp);

    /** callback functions **/
    sh.startDocument = start_doc;
    sh.endDocument = end_doc;
    sh.startElement = start_elt;
    sh.endElement = end_elt;
    sh.characters = chars;

    list = xbt_fifo_new();

    if (xmlSAXUserParseFile(&sh, &name, file) != 0) {
        printf("Error during file parsing.\n");
        return NULL;
    }

    return list;
}

