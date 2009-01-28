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

#include <string.h>
#include <unistd.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

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

/** File to store parsed data in wld format **/
static FILE* file;


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


static void start_doc(void *user_data) {
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

static void end_doc(void *user_data) {
    int i;

    for (i=0; i<4; i++) {
        free(infos[i]);
    }
    fprintf(stderr, "Done.\n");
}

static void start_elt(void *user_data, const xmlChar *name, const xmlChar **attrs) {
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
        fprintf( file, "%s %s %s 0 0 %s %s 0 0\n",
                 infos[0], infos[1], infos[2], infos[2], infos[3] );
#ifdef DEBUG
        fprintf( stderr, "%s %s %s 0 0 %s %s 0 0\n",
                 infos[0], infos[1], infos[2], infos[2], infos[3] );
#endif

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
        sprintf(str, "%.*s", len, ch);
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

int main(int argc, char** argv) {
    char* tmp = NULL;
    const char* sed_cmd = "sed -i '/wanted_resources/s/-l.*,.*=//g; \
                           /wanted_resources/s/&quot; //g'";
    xmlSAXHandler sh = { 0 };

    if (argc != 3) {
        fprintf(stderr, "Usage : %s <oarstat_file.xml> <output_file.wld>\n", argv[0]);
        return EXIT_FAILURE;
    }

    file = fopen(argv[2], "w");
    if ( file == NULL ) {
        fprintf(stderr, "Problem on file creation. Exit.\n");
        return EXIT_FAILURE;
    }

    tmp = calloc(strlen(sed_cmd) + strlen(argv[1]) + 2, sizeof(char));
    sprintf(tmp, "%s %s", sed_cmd, argv[1]);
    if (system(tmp) != 0) {
        fprintf(stderr, "Error during editing XML file (sed).\n");
        return EXIT_FAILURE;
    }
    free(tmp);

    /** Init des fonctions de traitement **/
    sh.startDocument = start_doc;
    sh.endDocument = end_doc;
    sh.startElement = start_elt;
    sh.endElement = end_elt;
    sh.characters = chars;

    // Parsing du document XML
    if (xmlSAXUserParseFile(&sh, NULL, argv[1]) != 0) {
        fprintf(stderr, "Problem during parsing. Exit.\n");
        return EXIT_FAILURE;
    }

    fclose( file );

    return EXIT_SUCCESS;
}

