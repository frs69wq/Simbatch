/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay and Ghislain Charrier, ENS Lyon.           */
/*                                                                          */
/* Copyright (c) 2007, Simbatch team. All rights reserved.                  */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/



/*
 * This file should be removes. They were disgned for running tests.
 * Those clients could move in the example directory.
 */

#ifndef _CLIENT_H_
#define _CLIENT_H_

int
SB_client(int argc, char **argv);

int
SB_file_client(int argc, char **argv);

int
SB_job_client(int argc, char **argv);

#endif
