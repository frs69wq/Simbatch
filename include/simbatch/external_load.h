/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, Gislain Charrier, ENS Lyon                */
/*                                                                          */
/* Copyright (c) 2007 Simbath team. All rights reserved.                    */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


/**
 * \file external load.h
 * Define the batch process and functions to configure it.
 * Every SB_process process is a MSG_process.
 */


#ifndef _EXTERNAL_LOAD_H_
#define _EXTERNAL_LOAD_H_


/** 
 * Request the name of the external load file.
 * External load is a load wich a SB_batch process is dealing with before a 
 * SB_client submit its own jobs.
 * The request is made by using a DOM parser and XML Queries provided by the
 * libxml2. This is an easy way for getting informations but it shows some
 * disadvantages :
 * - simgrid already uses xml parser with flexml => useless depency?
 * - DOM is cool for short XML file.
 * \todo See if the dependency could be removed by a better parsing of argc
 * and **argv in SB_external_load function.
 * \return the name of the file containing the load. 
 */ 
const char * 
SB_request_external_load(void);

/**
 * Request the name of the parser.
 * See SB_request_external_load() for remaks about the use of libxml2.
 * \return the name of the parser to use for creating the load from the
 * external loa file.
 */
const char *
SB_request_parser(void);

/**
 * MSG_process that handle the external load.
 * This MSG_process is created by the SB_batch process. The external_load is
 * just a special SB_client for the SB_batch process.
 * \param argc number of parameters in **argv.
 * \param **argv array containing the parameters.
 * \return a code error.
 */
int
SB_external_load(int argc, char **argv);

#endif
