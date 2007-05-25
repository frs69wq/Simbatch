/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _EXTERNAL_LOAD_H_
#define _EXTERNAL_LOAD_H_

/* Request the name of the external load file to the config */ 
const char * SB_request_external_load(void);

const char * SB_request_parser(void);

/* Process that handle the external load */
int SB_external_load(int argc, char ** argv);

#endif
